// =============================================================================
//  Module 11 -- demonstration : couverture structurelle et MC/DC.
// =============================================================================
#include <avio/types.hpp>
#include <cstdio>

#include "mod11/gpws.hpp"
#include "mod11/mcdc.hpp"

using avio::u32;
using avio::usize;
using mod11::DecisionRecorder;
using mod11::McdcReport;

namespace {

void title(const char* text) {
    std::printf("\n=== %s ===\n", text);
}

void run_mode4a(DecisionRecorder& recorder, bool c1, bool c2, bool c3, bool c4) {
    const bool conditions[4] = {c1, c2, c3, c4};
    (void)recorder.record(conditions, 4U, mod11::mode4a_decision(c1, c2, c3, c4));
}

void run_inhibition(DecisionRecorder& recorder, bool a, bool b, bool c) {
    const bool conditions[3] = {a, b, c};
    (void)recorder.record(conditions, 3U, mod11::inhibition_decision(a, b, c));
}

void print_set(const DecisionRecorder& recorder, const char* const* names) {
    std::printf("    #  ");
    for (usize index = 0U; index < recorder.condition_count(); ++index) {
        std::printf("%-6s", names[index]);
    }
    std::printf("| issue\n");
    std::printf("    ---");
    for (usize index = 0U; index < recorder.condition_count(); ++index) {
        std::printf("------");
    }
    std::printf("+-------\n");

    for (usize index = 0U; index < recorder.size(); ++index) {
        std::printf("    %zu  ", index);
        for (usize condition = 0U; condition < recorder.condition_count(); ++condition) {
            std::printf("%-6s", recorder.at(index).conditions[condition] ? "V" : "F");
        }
        std::printf("| %s\n", recorder.at(index).outcome ? "VRAI" : "faux");
    }
}

void print_report(const DecisionRecorder& recorder, const char* const* names) {
    const McdcReport report = mod11::analyze_mcdc(recorder);
    std::printf("\n    Analyse MC/DC :\n");
    std::printf("      issues observees : vrai=%s faux=%s\n",
                report.outcome_true_seen ? "oui" : "NON",
                report.outcome_false_seen ? "oui" : "NON");

    for (usize condition = 0U; condition < report.condition_count; ++condition) {
        if (report.condition_covered[condition]) {
            std::printf("      %-6s : COUVERTE   paire d'independance (%zu, %zu)\n",
                        names[condition], report.pair_first[condition],
                        report.pair_second[condition]);
        } else {
            std::printf("      %-6s : NON COUVERTE%s\n", names[condition],
                        report.condition_both_values[condition]
                            ? ""
                            : "  (et n'a pas pris ses deux valeurs)");
        }
    }
    std::printf("      -> %zu/%zu conditions couvertes, MC/DC %s\n", report.covered_count(),
                report.condition_count, report.is_complete() ? "ATTEINT" : "NON ATTEINT");
}

// -----------------------------------------------------------------------------
void criteria_by_level() {
    title("Quel critere de couverture pour quel niveau ?");
    std::printf("  DAL | statement | decision | MC/DC | couplage donnees/controle\n");
    std::printf("  ----+-----------+----------+-------+--------------------------\n");
    std::printf("   A  |    oui    |   oui    |  OUI  |          oui\n");
    std::printf("   B  |    oui    |   oui    |  non  |          oui\n");
    std::printf("   C  |    oui    |   non    |  non  |          oui\n");
    std::printf("   D  |    non    |   non    |  non  |          non\n");
    std::printf("   E  |    non    |   non    |  non  |          non\n");
    std::printf("\n  Objectifs A-7.5 (MC/DC), A-7.6 (decision), A-7.7 (statement),\n");
    std::printf("  A-7.8 (couplage donnees et controle -- module 12).\n");
    std::printf("\n  Passer de DAL B a DAL A, c'est essentiellement ajouter MC/DC.\n");
    std::printf("  C'est aussi ce qui explique l'ecart de cout entre les deux.\n");
}

// -----------------------------------------------------------------------------
void decision_is_not_enough() {
    title("Piege : 100 %% de couverture de DECISION, 0 %% de MC/DC");

    const char* const names[4] = {"alt", "vit", "train", "vol"};
    DecisionRecorder recorder(4U);
    run_mode4a(recorder, true, true, true, true);
    run_mode4a(recorder, false, false, false, false);

    std::printf("\n  Deux tests. La decision prend ses DEUX issues :\n");
    print_set(recorder, names);
    print_report(recorder, names);

    std::printf("\n  Couverture de decision : 100 %%. Couverture MC/DC : 0 %%.\n");
    std::printf("  Les deux evaluations different sur les QUATRE conditions a la\n");
    std::printf("  fois : impossible d'attribuer le changement d'issue a l'une\n");
    std::printf("  d'elles. Une condition pourrait etre inversee dans le code sans\n");
    std::printf("  qu'aucun test ne le detecte.\n");
}

// -----------------------------------------------------------------------------
void minimal_mcdc_set() {
    title("Le jeu MC/DC minimal d'une conjonction : N + 1 tests");

    const char* const names[4] = {"alt", "vit", "train", "vol"};
    DecisionRecorder recorder(4U);
    run_mode4a(recorder, true, true, true, true);
    run_mode4a(recorder, false, true, true, true);
    run_mode4a(recorder, true, false, true, true);
    run_mode4a(recorder, true, true, false, true);
    run_mode4a(recorder, true, true, true, false);

    print_set(recorder, names);
    print_report(recorder, names);

    std::printf("\n  5 tests au lieu de 16. Le gain croit vite :\n");
    std::printf("      N =  4 ->  5 tests au lieu de 16\n");
    std::printf("      N =  8 ->  9 tests au lieu de 256\n");
    std::printf("      N = 16 -> 17 tests au lieu de 65 536\n");
    std::printf("  C'est ce compromis qui a fait retenir MC/DC pour le DAL A.\n");
}

// -----------------------------------------------------------------------------
void mixed_decision() {
    title("Une decision MIXTE : A OU (B ET C)");

    const char* const names[3] = {"test", "appr", "plan"};
    DecisionRecorder recorder(3U);
    run_inhibition(recorder, false, false, true);
    run_inhibition(recorder, true, false, true);
    run_inhibition(recorder, false, true, true);
    run_inhibition(recorder, false, true, false);

    print_set(recorder, names);
    print_report(recorder, names);

    std::printf("\n  4 tests pour 3 conditions : l'optimum theorique N + 1.\n");
    std::printf("  Remarquez que l'evaluation 0 sert de reference pour DEUX paires,\n");
    std::printf("  et l'evaluation 2 pour une troisieme. Trouver un jeu minimal sur\n");
    std::printf("  une decision mixte n'a rien d'evident : c'est un vrai travail\n");
    std::printf("  d'ingenierie de test, souvent outille.\n");
}

// -----------------------------------------------------------------------------
void short_circuit() {
    title("Le piege du court-circuit");
    std::printf("  En C++, `&&` et `||` sont a COURT-CIRCUIT :\n\n");
    std::printf("      if (a && b) { ... }   // b n'est PAS evalue si a est faux\n\n");
    std::printf("  Consequence pour MC/DC : une condition non evaluee n'a pas pris\n");
    std::printf("  de valeur. Deux ecoles s'affrontent :\n\n");
    std::printf("    * MC/DC \"unique cause\"  : la paire d'independance ne differe\n");
    std::printf("      QUE par la condition etudiee. Definition d'origine, la plus\n");
    std::printf("      stricte. C'est celle qu'implemente mcdc.cpp.\n\n");
    std::printf("    * MC/DC \"masking\"       : on autorise d'autres conditions a\n");
    std::printf("      changer si l'on demontre qu'elles sont MASQUEES. Necessaire\n");
    std::printf("      des qu'une decision contient des conditions couplees.\n\n");
    std::printf("  PARADE DE CONCEPTION appliquee ici : evaluer les conditions dans\n");
    std::printf("  des variables nommees AVANT de former la decision.\n\n");
    std::printf("      const bool alt = radio_altitude < 500.0F;\n");
    std::printf("      const bool vit = airspeed < 190.0F;\n");
    std::printf("      return alt && vit && train && vol;\n\n");
    std::printf("  Toutes les conditions sont alors reellement evaluees, l'analyse\n");
    std::printf("  devient exacte, et le code est plus lisible. C'est ce que fait\n");
    std::printf("  gpws.cpp, et c'est a defendre en revue de conception.\n");
}

// -----------------------------------------------------------------------------
void measure_coverage() {
    title("Mesurer la couverture pour de vrai");
    std::printf("  L'analyseur de ce module raisonne sur les DECISIONS. Pour la\n");
    std::printf("  couverture d'INSTRUCTIONS et de BRANCHES, il faut instrumenter\n");
    std::printf("  le binaire. Sous Windows/MSVC, l'outil de reference est\n");
    std::printf("  OpenCppCoverage (gratuit, open source) :\n\n");
    std::printf("      .\\scripts\\coverage.ps1\n\n");
    std::printf("  Il produit un rapport HTML ligne par ligne. Voir le README du\n");
    std::printf("  module pour l'installation.\n\n");
    std::printf("  ATTENTION -- objectif A-7.9 : en DAL A, si le compilateur genere\n");
    std::printf("  du code objet NON tracable au code source (verifications\n");
    std::printf("  implicites, deroulement de boucle, table de saut), la couverture\n");
    std::printf("  doit etre demontree au niveau du CODE OBJET, pas du source.\n");
    std::printf("  C'est l'un des postes de cout les plus sous-estimes du DAL A.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 11 : couverture structurelle et MC/DC              #\n");
    std::printf("#############################################################\n");

    criteria_by_level();
    decision_is_not_enough();
    minimal_mcdc_set();
    mixed_decision();
    short_circuit();
    measure_coverage();

    std::printf("\nModule 11 termine.\n");
    return 0;
}
