// =============================================================================
//  Module 09 -- demonstration : exigences et tracabilite.
// =============================================================================
#include "mod09/altitude.hpp"

#include <avio/types.hpp>

#include <cstdio>
#include <limits>

using avio::f32;
using avio::usize;

namespace {

void titre(const char* texte) {
    std::printf("\n=== %s ===\n", texte);
}

// -----------------------------------------------------------------------------
void hierarchie_des_exigences() {
    titre("La hierarchie des exigences DO-178C");
    std::printf("  Exigences SYSTEME            (hors perimetre logiciel)\n");
    std::printf("         |\n");
    std::printf("         v\n");
    std::printf("  HLR -- Exigences de HAUT niveau      -> document SRD\n");
    std::printf("         CE QUE le logiciel doit faire. Jamais COMMENT.\n");
    std::printf("         ex. HLR-ADCALT-002 : domaine [100 ; 1100] hPa\n");
    std::printf("         |\n");
    std::printf("         v\n");
    std::printf("  LLR -- Exigences de BAS niveau + ARCHITECTURE -> document SDD\n");
    std::printf("         COMMENT. Assez detaillees pour coder directement.\n");
    std::printf("         ex. LLR-ADCALT-020 : h = 145366,45 x (1 - (p/1013,25)^0,190284)\n");
    std::printf("         |\n");
    std::printf("         v\n");
    std::printf("  CODE SOURCE  (annote @satisfies LLR-...)\n");
    std::printf("         |\n");
    std::printf("         v\n");
    std::printf("  CAS DE TEST  (TEST_REQ(..., \"LLR-...\"))\n");
    std::printf("\n  La tracabilite doit etre BIDIRECTIONNELLE : de l'exigence vers\n");
    std::printf("  le code ET du code vers l'exigence. Le second sens est celui qui\n");
    std::printf("  revele le code non justifie.\n");
}

// -----------------------------------------------------------------------------
void calcul_altitude() {
    titre("ADC-ALT en fonctionnement");

    const f32 pressions[8] = {1013.25F, 1000.0F, 950.0F, 850.0F,
                              700.0F,   500.0F,  300.0F, 200.0F};

    std::printf("  %-14s %14s\n", "pression (hPa)", "altitude (ft)");
    std::printf("  ------------------------------\n");
    for (usize index = 0U; index < 8U; ++index) {
        const mod07::Result<f32> resultat = mod09::pressure_altitude_feet(pressions[index]);
        if (resultat.is_ok()) {
            std::printf("  %14.2f %14.1f\n", static_cast<double>(pressions[index]),
                        static_cast<double>(resultat.value()));
        }
    }

    std::printf("\n  Effet du calage altimetrique (QNH) a 850 hPa :\n");
    const f32 calages[3] = {1003.25F, 1013.25F, 1023.25F};
    for (usize index = 0U; index < 3U; ++index) {
        const mod07::Result<f32> resultat =
            mod09::corrected_altitude_feet(850.0F, calages[index]);
        if (resultat.is_ok()) {
            std::printf("    QNH %7.2f hPa -> %9.1f ft\n", static_cast<double>(calages[index]),
                        static_cast<double>(resultat.value()));
        }
    }
    std::printf("\n  %.1f ft par hectopascal (LLR-ADCALT-033).\n",
                static_cast<double>(mod09::feet_per_hpa()));
}

// -----------------------------------------------------------------------------
void robustesse() {
    titre("Robustesse : chaque rejet a une cause identifiee");

    struct Cas {
        const char* libelle;
        f32 pression;
        f32 qnh;
    };
    const Cas cas[6] = {
        {"nominal", 850.0F, 1013.0F},
        {"pression trop basse", 50.0F, 1013.0F},
        {"pression trop haute", 1200.0F, 1013.0F},
        {"pression NaN", std::numeric_limits<f32>::quiet_NaN(), 1013.0F},
        {"calage hors domaine", 850.0F, 1200.0F},
        {"les DEUX invalides", 50.0F, 1200.0F}};

    for (usize index = 0U; index < 6U; ++index) {
        const mod07::Result<f32> resultat =
            mod09::corrected_altitude_feet(cas[index].pression, cas[index].qnh);
        if (resultat.is_ok()) {
            std::printf("  %-24s -> %.1f ft\n", cas[index].libelle,
                        static_cast<double>(resultat.value()));
        } else {
            std::printf("  %-24s -> REJET : %s\n", cas[index].libelle,
                        mod07::status_name(resultat.status()));
        }
    }
    std::printf("\n  Le dernier cas est instructif : les DEUX entrees sont invalides.\n");
    std::printf("  LLR-ADCALT-032 SPECIFIE que c'est le statut de la pression qui\n");
    std::printf("  remonte. Sans cette specification, le comportement dependrait de\n");
    std::printf("  l'implementation -- donc ne serait pas verifiable.\n");
}

// -----------------------------------------------------------------------------
void verifier_la_tracabilite() {
    titre("Verifier la tracabilite");
    std::printf("  Lancez, a la racine du depot :\n\n");
    std::printf("      python tools/trace_check.py\n\n");
    std::printf("  L'outil lit :\n");
    std::printf("    * les exigences declarees dans modules/*/requirements/*.md ;\n");
    std::printf("    * les annotations @satisfies du code source ;\n");
    std::printf("    * les TEST_REQ(...) des fichiers de test.\n\n");
    std::printf("  Il reconstruit la matrice et signale les quatre defauts :\n");
    std::printf("    1. exigence SANS code   -> non implementee\n");
    std::printf("    2. exigence SANS test   -> non verifiee\n");
    std::printf("    3. code SANS exigence   -> non justifie (code mort ?)\n");
    std::printf("    4. test SANS exigence   -> test orphelin\n\n");
    std::printf("  Ces quatre questions sont exactement celles que pose un auditeur.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 09 : exigences et tracabilite                      #\n");
    std::printf("#############################################################\n");

    hierarchie_des_exigences();
    calcul_altitude();
    robustesse();
    verifier_la_tracabilite();

    std::printf("\nModule 09 termine.\n");
    return 0;
}
