// =============================================================================
//  Module 07 -- demonstration : erreurs sans exceptions.
// =============================================================================
#include "mod07/arinc429.hpp"
#include "mod07/result.hpp"

#include <avio/types.hpp>

#include <cstdio>

using avio::i32;
using avio::u8;
using avio::u32;
using avio::usize;
using mod07::Result;
using mod07::SignStatus;
using mod07::Status;

namespace {

void titre(const char* texte) {
    std::printf("\n=== %s ===\n", texte);
}

// -----------------------------------------------------------------------------
void pourquoi_pas_d_exceptions() {
    titre("Pourquoi les exceptions sont interdites en avionique");
    std::printf("  1. TEMPS D'EXECUTION NON BORNE\n");
    std::printf("     Le deroulement de pile parcourt des tables generees par le\n");
    std::printf("     compilateur. Aucun outil d'analyse WCET du marche ne sait le\n");
    std::printf("     borner utilement. Or le WCET doit etre DEMONTRE.\n\n");
    std::printf("  2. FLOT DE CONTROLE IMPLICITE\n");
    std::printf("     `f(); g();` : si f peut lancer, g peut ne jamais s'executer.\n");
    std::printf("     Rien dans le code ne l'indique. L'analyse de couplage de\n");
    std::printf("     controle (module 12) devient tres difficile.\n\n");
    std::printf("  3. ALLOCATION DYNAMIQUE\n");
    std::printf("     L'objet exception est alloue sur un tas dedie -> interdit.\n\n");
    std::printf("  4. TAILLE DU CODE\n");
    std::printf("     Les tables de deroulement pesent 10 a 30 %% du binaire.\n\n");
    std::printf("  5. DO-332, vulnerabilite 6 : objectifs supplementaires.\n\n");
    std::printf("  En pratique : /EHs-c- (MSVC) ou -fno-exceptions (GCC/Clang).\n");
    std::printf("  ATTENTION : `noexcept` n'EMPECHE pas de lancer, il PROMET de ne\n");
    std::printf("  pas le faire. Si une exception s'echappe malgre tout, c'est\n");
    std::printf("  std::terminate() -- donc un redemarrage du calculateur en vol.\n");
}

// -----------------------------------------------------------------------------
void motif_result() {
    titre("Le motif Result<T> : une valeur OU une erreur");

    const Result<i32> par_defaut;
    const Result<i32> succes = Result<i32>::ok(42);
    const Result<i32> echec = Result<i32>::error(Status::Timeout);

    std::printf("  Result<i32>{}                 -> is_ok=%-5s statut=%s\n",
                par_defaut.is_ok() ? "true" : "false",
                mod07::status_name(par_defaut.status()));
    std::printf("  Result<i32>::ok(42)           -> is_ok=%-5s valeur=%d\n",
                succes.is_ok() ? "true" : "false", succes.value());
    std::printf("  Result<i32>::error(Timeout)   -> is_ok=%-5s statut=%s, value_or(-1)=%d\n",
                echec.is_ok() ? "true" : "false", mod07::status_name(echec.status()),
                echec.value_or(-1));

    std::printf("\n  Point de conception : le DEFAUT est une ERREUR (NonDisponible).\n");
    std::printf("  Un Result oublie ne peut donc jamais passer pour un succes.\n");
    std::printf("  C'est la difference entre un defaut sur et un defaut discret.\n");

    std::printf("\n  sizeof(Result<i32>) = %zu octets : une valeur + un statut,\n",
                sizeof(Result<i32>));
    std::printf("  sans union ni placement new. Disposition triviale, donc\n");
    std::printf("  entierement analysable. En avionique, la simplicite d'analyse\n");
    std::printf("  prime sur l'economie de quelques octets.\n");
}

// -----------------------------------------------------------------------------
void decodage_arinc() {
    titre("ARINC 429 : un decodeur complet, sans une seule exception");

    struct Scenario {
        const char* libelle;
        u32 mot;
    };

    u32 mot_altere =
        mod07::encode(mod07::kLabelAltitude, 0U, 12000U, SignStatus::NormalOperation);
    mot_altere ^= 0x00001000U;

    const Scenario scenarios[6] = {
        {"altitude 12000 ft, normal",
         mod07::encode(mod07::kLabelAltitude, 0U, 12000U, SignStatus::NormalOperation)},
        {"altitude -500 ft, normal",
         mod07::encode(mod07::kLabelAltitude, 0U, 524288U - 500U, SignStatus::NormalOperation)},
        {"bit altere en transmission", mot_altere},
        {"label non traite (42)",
         mod07::encode(u8{42U}, 0U, 12000U, SignStatus::NormalOperation)},
        {"source en panne",
         mod07::encode(mod07::kLabelAltitude, 0U, 12000U, SignStatus::FailureWarning)},
        {"altitude 200000 ft (aberrante)",
         mod07::encode(mod07::kLabelAltitude, 0U, 200000U, SignStatus::NormalOperation)}};

    mod07::StatusCounters compteurs;
    compteurs.reset();

    std::printf("  %-32s %-12s %s\n", "scenario", "mot brut", "resultat");
    std::printf("  ---------------------------------------------------------------------\n");
    for (usize index = 0U; index < 6U; ++index) {
        const Result<i32> resultat = mod07::extract_altitude_feet(scenarios[index].mot);
        compteurs.record(resultat.status());

        if (resultat.is_ok()) {
            std::printf("  %-32s 0x%08X   %d ft\n", scenarios[index].libelle,
                        scenarios[index].mot, resultat.value());
        } else {
            std::printf("  %-32s 0x%08X   ERREUR : %s\n", scenarios[index].libelle,
                        scenarios[index].mot, mod07::status_name(resultat.status()));
        }
    }

    std::printf("\n  Surveillance (BITE) : %u anomalie(s), dominante = %s\n",
                compteurs.total_faults(), mod07::status_name(compteurs.dominant_fault()));
    std::printf("\n  Une erreur qui n'est ni traitee ni COMPTEE est une erreur\n");
    std::printf("  invisible. Tout calculateur certifie tient ces compteurs, relus\n");
    std::printf("  par la maintenance au sol.\n");
}

// -----------------------------------------------------------------------------
void code_mort_et_desactive() {
    titre("Code mort, code desactive : deux notions a ne pas confondre");
    std::printf("  CODE MORT (dead code) -- DO-178C 6.4.4.3\n");
    std::printf("    Du code qui ne peut JAMAIS s'executer, quelle que soit\n");
    std::printf("    l'entree. Ce n'est pas une exigence, c'est un DEFAUT.\n");
    std::printf("    Action requise : le SUPPRIMER, et analyser pourquoi il\n");
    std::printf("    existait (souvent : une exigence oubliee ou un bug).\n\n");
    std::printf("  CODE DESACTIVE (deactivated code)\n");
    std::printf("    Du code INTENTIONNELLEMENT non executable dans cette\n");
    std::printf("    configuration : option non retenue, variante d'un autre\n");
    std::printf("    programme, mode de maintenance au sol.\n");
    std::printf("    Il est ACCEPTABLE, a condition de :\n");
    std::printf("      * l'identifier explicitement dans la conception ;\n");
    std::printf("      * demontrer qu'il ne peut PAS etre active par erreur ;\n");
    std::printf("      * justifier son absence de couverture.\n\n");
    std::printf("  LE PIEGE DU CODE DEFENSIF\n");
    std::printf("    `if (p == nullptr) { ... }` alors que p ne peut pas etre nul :\n");
    std::printf("    excellente pratique ailleurs, DEFAUT ici. Cette branche n'est\n");
    std::printf("    tracee a aucune exigence et ne sera jamais couverte.\n");
    std::printf("    La bonne demarche : ECRIRE L'EXIGENCE DE ROBUSTESSE d'abord,\n");
    std::printf("    puis le code, puis le test qui l'atteint.\n");
    std::printf("    C'est pourquoi chaque cas d'erreur de ce module porte un\n");
    std::printf("    identifiant LLR-M07-0xx et un test qui l'exerce.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 07 : gestion d'erreurs sans exceptions             #\n");
    std::printf("#############################################################\n");

    pourquoi_pas_d_exceptions();
    motif_result();
    decodage_arinc();
    code_mort_et_desactive();

    std::printf("\nModule 07 termine.\n");
    return 0;
}
