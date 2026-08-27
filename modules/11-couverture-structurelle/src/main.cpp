// =============================================================================
//  Module 11 -- demonstration : couverture structurelle et MC/DC.
// =============================================================================
#include "mod11/gpws.hpp"
#include "mod11/mcdc.hpp"

#include <avio/types.hpp>

#include <cstdio>

using avio::u32;
using avio::usize;
using mod11::DecisionRecorder;
using mod11::McdcReport;

namespace {

void titre(const char* texte) {
    std::printf("\n=== %s ===\n", texte);
}

void executer_mode4a(DecisionRecorder& enregistreur, bool c1, bool c2, bool c3, bool c4) {
    const bool conditions[4] = {c1, c2, c3, c4};
    (void)enregistreur.record(conditions, 4U, mod11::mode4a_decision(c1, c2, c3, c4));
}

void executer_inhibition(DecisionRecorder& enregistreur, bool a, bool b, bool c) {
    const bool conditions[3] = {a, b, c};
    (void)enregistreur.record(conditions, 3U, mod11::inhibition_decision(a, b, c));
}

void afficher_jeu(const DecisionRecorder& enregistreur, const char* const* noms) {
    std::printf("    #  ");
    for (usize index = 0U; index < enregistreur.condition_count(); ++index) {
        std::printf("%-6s", noms[index]);
    }
    std::printf("| issue\n");
    std::printf("    ---");
    for (usize index = 0U; index < enregistreur.condition_count(); ++index) {
        std::printf("------");
    }
    std::printf("+-------\n");

    for (usize index = 0U; index < enregistreur.size(); ++index) {
        std::printf("    %zu  ", index);
        for (usize condition = 0U; condition < enregistreur.condition_count(); ++condition) {
            std::printf("%-6s", enregistreur.at(index).conditions[condition] ? "V" : "F");
        }
        std::printf("| %s\n", enregistreur.at(index).outcome ? "VRAI" : "faux");
    }
}

void afficher_rapport(const DecisionRecorder& enregistreur, const char* const* noms) {
    const McdcReport rapport = mod11::analyze_mcdc(enregistreur);
    std::printf("\n    Analyse MC/DC :\n");
    std::printf("      issues observees : vrai=%s faux=%s\n",
                rapport.outcome_true_seen ? "oui" : "NON",
                rapport.outcome_false_seen ? "oui" : "NON");

    for (usize condition = 0U; condition < rapport.condition_count; ++condition) {
        if (rapport.condition_covered[condition]) {
            std::printf("      %-6s : COUVERTE   paire d'independance (%zu, %zu)\n",
                        noms[condition], rapport.pair_first[condition],
                        rapport.pair_second[condition]);
        } else {
            std::printf("      %-6s : NON COUVERTE%s\n", noms[condition],
                        rapport.condition_both_values[condition]
                            ? ""
                            : "  (et n'a pas pris ses deux valeurs)");
        }
    }
    std::printf("      -> %zu/%zu conditions couvertes, MC/DC %s\n", rapport.covered_count(),
                rapport.condition_count, rapport.is_complete() ? "ATTEINT" : "NON ATTEINT");
}

// -----------------------------------------------------------------------------
void criteres_par_niveau() {
    titre("Quel critere de couverture pour quel niveau ?");
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
void decision_ne_suffit_pas() {
    titre("Piege : 100 %% de couverture de DECISION, 0 %% de MC/DC");

    const char* const noms[4] = {"alt", "vit", "train", "vol"};
    DecisionRecorder enregistreur(4U);
    executer_mode4a(enregistreur, true, true, true, true);
    executer_mode4a(enregistreur, false, false, false, false);

    std::printf("\n  Deux tests. La decision prend ses DEUX issues :\n");
    afficher_jeu(enregistreur, noms);
    afficher_rapport(enregistreur, noms);

    std::printf("\n  Couverture de decision : 100 %%. Couverture MC/DC : 0 %%.\n");
    std::printf("  Les deux evaluations different sur les QUATRE conditions a la\n");
    std::printf("  fois : impossible d'attribuer le changement d'issue a l'une\n");
    std::printf("  d'elles. Une condition pourrait etre inversee dans le code sans\n");
    std::printf("  qu'aucun test ne le detecte.\n");
}

// -----------------------------------------------------------------------------
void jeu_mcdc_minimal() {
    titre("Le jeu MC/DC minimal d'une conjonction : N + 1 tests");

    const char* const noms[4] = {"alt", "vit", "train", "vol"};
    DecisionRecorder enregistreur(4U);
    executer_mode4a(enregistreur, true, true, true, true);
    executer_mode4a(enregistreur, false, true, true, true);
    executer_mode4a(enregistreur, true, false, true, true);
    executer_mode4a(enregistreur, true, true, false, true);
    executer_mode4a(enregistreur, true, true, true, false);

    afficher_jeu(enregistreur, noms);
    afficher_rapport(enregistreur, noms);

    std::printf("\n  5 tests au lieu de 16. Le gain croit vite :\n");
    std::printf("      N =  4 ->  5 tests au lieu de 16\n");
    std::printf("      N =  8 ->  9 tests au lieu de 256\n");
    std::printf("      N = 16 -> 17 tests au lieu de 65 536\n");
    std::printf("  C'est ce compromis qui a fait retenir MC/DC pour le DAL A.\n");
}

// -----------------------------------------------------------------------------
void decision_mixte() {
    titre("Une decision MIXTE : A OU (B ET C)");

    const char* const noms[3] = {"test", "appr", "plan"};
    DecisionRecorder enregistreur(3U);
    executer_inhibition(enregistreur, false, false, true);
    executer_inhibition(enregistreur, true, false, true);
    executer_inhibition(enregistreur, false, true, true);
    executer_inhibition(enregistreur, false, true, false);

    afficher_jeu(enregistreur, noms);
    afficher_rapport(enregistreur, noms);

    std::printf("\n  4 tests pour 3 conditions : l'optimum theorique N + 1.\n");
    std::printf("  Remarquez que l'evaluation 0 sert de reference pour DEUX paires,\n");
    std::printf("  et l'evaluation 2 pour une troisieme. Trouver un jeu minimal sur\n");
    std::printf("  une decision mixte n'a rien d'evident : c'est un vrai travail\n");
    std::printf("  d'ingenierie de test, souvent outille.\n");
}

// -----------------------------------------------------------------------------
void court_circuit() {
    titre("Le piege du court-circuit");
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
void mesurer_la_couverture() {
    titre("Mesurer la couverture pour de vrai");
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

    criteres_par_niveau();
    decision_ne_suffit_pas();
    jeu_mcdc_minimal();
    decision_mixte();
    court_circuit();
    mesurer_la_couverture();

    std::printf("\nModule 11 termine.\n");
    return 0;
}
