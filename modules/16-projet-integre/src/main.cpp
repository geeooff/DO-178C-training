// =============================================================================
//  FQMS -- simulation d'un vol complet.
// =============================================================================
#include <avio/types.hpp>
#include <cstdio>

#include "mod16/fqms.hpp"

using avio::i32;
using avio::u32;
using avio::usize;
using mod16::CycleReport;
using mod16::FuelSystem;
using mod16::TankId;

namespace {

void titre(const char* texte) {
    std::printf("\n=== %s ===\n", texte);
}

/// Une phase du profil de vol.
struct Phase {
    const char* libelle;
    i32 gauche;
    i32 central;
    i32 droite;
    usize cycles;
};

void afficher_entete() {
    std::printf("  %-30s %8s %8s %8s %9s %8s %-7s %-7s %s\n", "phase", "gauche", "central",
                "droite", "total", "ecart", "BAS", "DESEQ", "statut");
    std::printf(
        "  ---------------------------------------------------------"
        "--------------------------------------\n");
}

void afficher_ligne(const char* libelle, const CycleReport& rapport) {
    std::printf("  %-30s %8.0f %8.0f %8.0f %9.0f %8.0f %-7s %-7s %s\n", libelle,
                static_cast<double>(rapport.tank_quantity[0].kilograms()),
                static_cast<double>(rapport.tank_quantity[1].kilograms()),
                static_cast<double>(rapport.tank_quantity[2].kilograms()),
                static_cast<double>(rapport.total.kilograms()),
                static_cast<double>(rapport.wing_imbalance.kilograms()),
                rapport.low_fuel_alert ? "ALERTE" : "-", rapport.imbalance_alert ? "ALERTE" : "-",
                mod07::status_name(rapport.status));
}

// -----------------------------------------------------------------------------
void presentation() {
    titre("FQMS -- Fuel Quantity Management System");
    std::printf("  Systeme de gestion de la quantite de carburant d'un birecteur.\n");
    std::printf("  Trois reservoirs : aile gauche 5000 kg, central 8000 kg,\n");
    std::printf("  aile droite 5000 kg. Total 18 000 kg.\n\n");
    std::printf("  NIVEAU : DAL B.\n");
    std::printf("  Justification (analyse de securite systeme, ARP4761) : une\n");
    std::printf("  indication de quantite erronee PAR EXCES peut conduire l'equipage\n");
    std::printf("  a decoller avec un carburant insuffisant, donc a une panne seche\n");
    std::printf("  en vol -- condition de panne DANGEREUSE. Vol Air Canada 143,\n");
    std::printf("  1983 : un Boeing 767 a plane 30 km jusqu'a un aerodrome desaffecte.\n\n");
    std::printf("  ALERTES SURVEILLEES :\n");
    std::printf("    BAS NIVEAU    : total < 1500 kg pendant 5 cycles\n");
    std::printf("                    effacement au-dessus de 1700 kg (hysteresis 200 kg)\n");
    std::printf("    DESEQUILIBRE  : |gauche - droite| > 500 kg pendant 5 cycles\n");
    std::printf("                    effacement sous 400 kg (hysteresis 100 kg)\n");
}

// -----------------------------------------------------------------------------
void profil_de_vol() {
    titre("Profil de vol nominal");

    FuelSystem systeme;
    if (!FuelSystem::create(mod16::default_config(), systeme)) {
        std::printf("  ERREUR : configuration refusee.\n");
        return;
    }

    const Phase phases[8] = {{"1. Avant vol, pleins faits", 4095, 4095, 4095, 5U},
                             {"2. Montee", 4095, 3276, 4095, 5U},
                             {"3. Croisiere, central se vide", 4095, 819, 4095, 5U},
                             {"4. Central vide", 4095, 0, 4095, 5U},
                             {"5. Transfert dissymetrique", 4095, 0, 3276, 8U},
                             {"6. Equilibrage par l'equipage", 3276, 0, 3276, 8U},
                             {"7. Descente", 819, 0, 819, 5U},
                             {"8. Approche, reserve", 0, 717, 0, 8U}};

    afficher_entete();
    for (usize index = 0U; index < 8U; ++index) {
        const i32 mesures[3] = {phases[index].gauche, phases[index].central, phases[index].droite};
        CycleReport rapport;
        for (usize cycle = 0U; cycle < phases[index].cycles; ++cycle) {
            rapport = systeme.update(mesures);
        }
        afficher_ligne(phases[index].libelle, rapport);
    }

    std::printf("\n  Phase 5 : le desequilibre de 1000 kg leve l'alerte apres\n");
    std::printf("  5 cycles de confirmation. Phase 6 : l'equipage retablit\n");
    std::printf("  l'equilibre, l'alerte s'efface apres 5 cycles.\n");
    std::printf("  Phase 8 : la quantite passe sous 1500 kg, l'alerte bas niveau\n");
    std::printf("  se leve.\n");
    std::printf("\n  Cycles traites : %u\n", systeme.cycle_count());
}

// -----------------------------------------------------------------------------
void panne_de_jauge() {
    titre("Panne de jauge : le cas qui justifie le gel des alertes");

    FuelSystem systeme;
    (void)FuelSystem::create(mod16::default_config(), systeme);

    afficher_entete();

    // Situation de depart : 3000 kg au total, au-dessus du seuil.
    const i32 nominal[3] = {246, 819, 246};
    CycleReport rapport;
    for (usize cycle = 0U; cycle < 10U; ++cycle) {
        rapport = systeme.update(nominal);
    }
    afficher_ligne("Nominal, 2200 kg", rapport);

    // La jauge centrale tombe en panne.
    const i32 avec_panne[3] = {246, -1, 246};
    for (usize cycle = 0U; cycle < 20U; ++cycle) {
        rapport = systeme.update(avec_panne);
    }
    afficher_ligne("Jauge centrale en panne", rapport);

    std::printf("\n  La quantite VISIBLE est tombee a %.0f kg, soit sous le seuil de\n",
                static_cast<double>(rapport.total.kilograms()));
    std::printf("  1500 kg. Et pourtant AUCUNE alerte bas niveau.\n\n");
    std::printf("  POURQUOI : la quantite n'est pas mesurable (HLR-FQMS-032). Un\n");
    std::printf("  echantillon non fini est transmis au moniteur d'alerte, qui\n");
    std::printf("  l'ignore et GELE son etat -- comportement specifie par\n");
    std::printf("  LLR-ALERT-050 du module 10, et reutilise ici a dessein.\n\n");
    std::printf("  SANS ce gel, l'equipage verrait une alerte BAS NIVEAU alors que\n");
    std::printf("  l'avion a 2200 kg a bord, et se derouterait sans raison. Une\n");
    std::printf("  fausse alerte a effet operationnel majeur est un defaut de\n");
    std::printf("  securite, au meme titre qu'une alerte manquante.\n\n");
    std::printf("  Le statut passe a \"%s\" : l'equipage SAIT que la quantite\n",
                mod07::status_name(rapport.status));
    std::printf("  affichee est incomplete. C'est HLR-FQMS-011.\n");

    std::printf("\n  Compteurs de maintenance (BITE) :\n");
    const TankId reservoirs[3] = {TankId::Left, TankId::Center, TankId::Right};
    for (usize index = 0U; index < 3U; ++index) {
        std::printf("    %-12s : %u rejet(s)\n", mod16::tank_name(reservoirs[index]),
                    systeme.fault_count(reservoirs[index]));
    }
}

// -----------------------------------------------------------------------------
void bilan_de_la_formation() {
    titre("Ce que ce composant rassemble");
    std::printf("  module 01  types de largeur fixe, arithmetique bornee\n");
    std::printf("  module 02  const-correctness, Span, pas d'arithmetique de pointeur\n");
    std::printf("  module 03  regle de 0 : aucune ressource a liberer\n");
    std::printf("  module 04  Mass : type fort, invariant, fabrique validante\n");
    std::printf("  module 05  hierarchie plate : AUCUNE fonction virtuelle\n");
    std::printf("  module 06  constantes constexpr, pas d'instanciation superflue\n");
    std::printf("  module 07  Result<T> et Status : aucune exception\n");
    std::printf("  module 08  aucune allocation dynamique, occupation RAM constante\n");
    std::printf("  module 09  SRD et SDD complets, annotations @satisfies\n");
    std::printf("  module 10  AlertMonitor reutilise : hysteresis et anti-rebond\n");
    std::printf("  module 11  decisions extraites en fonctions pures de booleens\n");
    std::printf("  module 12  couplage explicite, dependances par parametre\n");
    std::printf("  module 13  standard de codage applique, zero deviation\n");
    std::printf("  module 14  identite et integrite du logiciel\n");
    std::printf("  module 15  arithmetique entiere, cycle a WCET constant\n");

    std::printf("\n  sizeof(FuelSystem) = %zu octets -- constante, calculable,\n",
                sizeof(FuelSystem));
    std::printf("  inscriptible au budget memoire du calculateur.\n");

    std::printf("\n  VERIFIER LE DOSSIER :\n");
    std::printf("    python tools/trace_check.py\n");
    std::printf("    python tools/config_index.py\n");
    std::printf("    ctest --preset debug --output-on-failure\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  PROJET INTEGRE : FQMS (Fuel Quantity Management System)   #\n");
    std::printf("#############################################################\n");

    presentation();
    profil_de_vol();
    panne_de_jauge();
    bilan_de_la_formation();

    std::printf("\nProjet integre termine. Fin de la formation.\n");
    return 0;
}
