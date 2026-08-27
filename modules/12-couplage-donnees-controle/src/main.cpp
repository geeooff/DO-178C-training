// =============================================================================
//  Module 12 -- demonstration : couplage donnees et couplage controle.
// =============================================================================
#include "mod12/chain.hpp"
#include "mod12/coupling_trace.hpp"

#include <avio/types.hpp>

#include <cstdio>

using avio::i32;
using avio::u32;
using avio::usize;
using mod12::CouplingTrace;
using mod12::CycleOutcome;
using mod12::Interface;

namespace {

void titre(const char* texte) {
    std::printf("\n=== %s ===\n", texte);
}

// -----------------------------------------------------------------------------
void deux_notions() {
    titre("Deux notions a ne pas confondre");
    std::printf("  COUPLAGE DE DONNEES (data coupling)\n");
    std::printf("    \"La dependance d'un composant vis-a-vis de donnees qui ne sont\n");
    std::printf("     pas exclusivement sous son controle.\"\n");
    std::printf("    -> ce qui TRANSITE : parametres, retours, globales, memoire\n");
    std::printf("       partagee, messages de bus.\n");
    std::printf("    -> question type : \"cette valeur est-elle bien en metres a\n");
    std::printf("       l'arrivee comme au depart ?\"\n\n");
    std::printf("  COUPLAGE DE CONTROLE (control coupling)\n");
    std::printf("    \"La maniere ou le degre par lequel un composant influence\n");
    std::printf("     l'execution d'un autre.\"\n");
    std::printf("    -> QUI appelle QUI, dans quel ORDRE, sous quelle CONDITION.\n");
    std::printf("    -> question type : \"reset() est-il vraiment appele apres trois\n");
    std::printf("       rejets, et jamais avant ?\"\n\n");
    std::printf("  POURQUOI UN OBJECTIF SEPARE (A-7.8) ?\n");
    std::printf("    On peut atteindre 100 %% de MC/DC sur CHAQUE composant pris\n");
    std::printf("    isolement, et n'avoir JAMAIS teste leur assemblage. Ordre\n");
    std::printf("    d'appel inverse, donnee non initialisee au premier cycle,\n");
    std::printf("    unite non convertie a la frontiere : ces defauts n'existent\n");
    std::printf("    qu'a l'integration, et ce sont les plus couteux a corriger.\n");
    std::printf("\n  Requis en DAL A, B et C. Pas en D ni E.\n");
}

// -----------------------------------------------------------------------------
void matrices() {
    titre("Les deux matrices, livrables de l'analyse");
    std::printf("  MATRICE DE COUPLAGE DE CONTROLE\n");
    std::printf("  --------------------------------------------------------------\n");
    std::printf("   #  | appelant   | appele                | condition\n");
    std::printf("  ----+------------+-----------------------+--------------------\n");
    std::printf("   I1 | Supervisor | Acquisition::read()   | inconditionnel\n");
    std::printf("   I2 | Supervisor | Filter::push()        | lecture reussie\n");
    std::printf("   I3 | Supervisor | Filter::average()     | lecture reussie\n");
    std::printf("   I4 | Supervisor | Filter::reset()       | 3 rejets CONSECUTIFS\n");
    std::printf("\n  I4 est le cas critique : couplage CONDITIONNEL declenche par une\n");
    std::printf("  SEQUENCE. Aucun test unitaire ne peut l'exercer.\n\n");

    std::printf("  MATRICE DE COUPLAGE DE DONNEES\n");
    std::printf("  --------------------------------------------------------------\n");
    std::printf("   #  | producteur  | consommateur | donnee    | unite   | domaine\n");
    std::printf("  ----+-------------+--------------+-----------+---------+---------\n");
    std::printf("   D1 | Acquisition | Supervisor   | mesure    | capteur | 0..1023,75\n");
    std::printf("   D2 | Supervisor  | Filter       | echantill.| capteur | idem D1\n");
    std::printf("   D3 | Filter      | Supervisor   | moyenne   | capteur | 0..1023,75\n");
    std::printf("\n  La colonne UNITE n'est pas decorative : c'est a la frontiere D1\n");
    std::printf("  que l'echelle est fixee (0,25 unite par point). Une erreur ici se\n");
    std::printf("  propage silencieusement dans toute la chaine aval.\n");
}

// -----------------------------------------------------------------------------
void demonstration_instrumentee() {
    titre("Demontrer le couplage par instrumentation");

    mod12::Acquisition acquisition_reelle;
    mod12::Filter filtre_reel;
    mod12::TracingAcquisition acquisition{acquisition_reelle};
    mod12::TracingFilter filtre{filtre_reel};
    mod12::TracedSupervisor superviseur{acquisition, filtre};

    CouplingTrace::reset();

    std::printf("\n  Scenario : 5 cycles valides, puis 3 lectures en erreur.\n\n");
    std::printf("    cycle | brut  | statut          | filtre  | alerte | interfaces\n");
    std::printf("    ------+-------+-----------------+---------+--------+-----------\n");

    const i32 profil[8] = {2000, 2000, 2000, 2000, 2000, -1, -1, -1};
    // Etat local, pas de variable statique cachee : on veut pouvoir
    // relancer cette demonstration sans effet de bord residuel.
    u32 precedent[4] = {0U, 0U, 0U, 0U};
    for (usize cycle = 0U; cycle < 8U; ++cycle) {
        acquisition.set_raw(profil[cycle]);
        const CycleOutcome resultat = superviseur.cycle();

        char interfaces[8] = {'-', '-', '-', '-', '\0', '\0', '\0', '\0'};
        for (avio::u8 index = 0U; index < 4U; ++index) {
            const u32 courant = CouplingTrace::call_count(static_cast<Interface>(index));
            if (courant != precedent[index]) {
                interfaces[index] = static_cast<char>('1' + index);
                precedent[index] = courant;
            }
        }

        std::printf("    %5zu | %5d | %-15s | %7.1f | %-6s | I%s\n", cycle + 1U, profil[cycle],
                    mod07::status_name(resultat.status),
                    static_cast<double>(resultat.filtered_value),
                    resultat.alert ? "OUI" : "non", interfaces);
    }

    std::printf("\n  Bilan des appels par interface :\n");
    for (avio::u8 index = 0U; index < CouplingTrace::kInterfaceCount; ++index) {
        const Interface interface = static_cast<Interface>(index);
        std::printf("    %-38s : %u appel(s)%s\n", mod12::interface_name(interface),
                    CouplingTrace::call_count(interface),
                    (CouplingTrace::call_count(interface) == 0U) ? "   <-- NON EXERCEE" : "");
    }

    std::printf("\n  Toutes les interfaces exercees : %s\n",
                CouplingTrace::all_interfaces_exercised() ? "OUI (objectif A-7.8 demontre)"
                                                          : "NON");

    std::printf("\n  Donnees echangees (couplage de donnees) :\n");
    const usize a_afficher = (CouplingTrace::data_count() < 6U) ? CouplingTrace::data_count() : 6U;
    for (usize index = 0U; index < a_afficher; ++index) {
        Interface interface = Interface::Count;
        avio::f32 valeur = 0.0F;
        if (CouplingTrace::data_at(index, interface, valeur)) {
            std::printf("    %-38s : %.2f\n", mod12::interface_name(interface),
                        static_cast<double>(valeur));
        }
    }
    std::printf("    ... (%zu echantillons au total)\n", CouplingTrace::data_count());
}

// -----------------------------------------------------------------------------
void reduire_le_couplage() {
    titre("Reduire le couplage : les regles qui paient");
    std::printf("  1. AUCUNE VARIABLE GLOBALE MUTABLE partagee entre composants.\n");
    std::printf("     Une globale cree un couplage de donnees INVISIBLE dans les\n");
    std::printf("     signatures : la matrice devient impossible a etablir par\n");
    std::printf("     lecture. C'est la regle la plus rentable du lot.\n\n");
    std::printf("  2. DEPENDANCES EXPLICITES dans le type.\n");
    std::printf("       template <typename AcquisitionT, typename FilterT>\n");
    std::printf("       class Supervisor { ... };\n");
    std::printf("     Le couplage est dans la signature. Bonus : les tests peuvent\n");
    std::printf("     substituer des composants instrumentes SANS toucher au code de\n");
    std::printf("     production -- le binaire verifie reste le binaire embarque.\n");
    std::printf("     Ce n'est PAS le cas d'une instrumentation par #ifdef.\n\n");
    std::printf("  3. INTERFACES ETROITES.\n");
    std::printf("     Moins de fonctions exposees = moins d'interfaces a exercer.\n");
    std::printf("     Chaque methode publique est une ligne de la matrice.\n\n");
    std::printf("  4. PAS DE RAPPEL (callback) NON RESOLU STATIQUEMENT.\n");
    std::printf("     Un pointeur de fonction rend le graphe d'appel indeterminable,\n");
    std::printf("     donc l'analyse de couplage de controle -- et l'analyse de pile\n");
    std::printf("     du module 08 -- impossibles.\n\n");
    std::printf("  5. ORDRE D'APPEL SPECIFIE dans le SDD, pas seulement dans le code.\n");
    std::printf("     \"apres 3 rejets consecutifs\" est une EXIGENCE (LLR-CHAIN-031),\n");
    std::printf("     donc testable et tracable.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 12 : couplage donnees et couplage controle         #\n");
    std::printf("#############################################################\n");

    deux_notions();
    matrices();
    demonstration_instrumentee();
    reduire_le_couplage();

    std::printf("\nModule 12 termine.\n");
    return 0;
}
