#include <microtest/microtest.hpp>

#include "mod11/gpws.hpp"
#include "mod11/mcdc.hpp"

#include <avio/types.hpp>

using avio::usize;
using mod11::DecisionRecorder;
using mod11::McdcReport;
using mod11::Mode4aConditions;
using mod11::Mode4aInputs;

namespace {

/// Applique un vecteur de conditions a la decision mode 4A et l'enregistre.
void executer_mode4a(DecisionRecorder& enregistreur, bool c1, bool c2, bool c3, bool c4) noexcept {
    const bool conditions[4] = {c1, c2, c3, c4};
    const bool issue = mod11::mode4a_decision(c1, c2, c3, c4);
    (void)enregistreur.record(conditions, 4U, issue);
}

void executer_inhibition(DecisionRecorder& enregistreur, bool a, bool b, bool c) noexcept {
    const bool conditions[3] = {a, b, c};
    const bool issue = mod11::inhibition_decision(a, b, c);
    (void)enregistreur.record(conditions, 3U, issue);
}

}  // namespace

// =============================================================================
//  1. Evaluation des conditions (valeurs limites)
// =============================================================================
TEST_REQ(Conditions, seuils_exacts, "LLR-GPWS-010") {
    Mode4aInputs entrees;
    entrees.radio_altitude_ft = 500.0F;
    entrees.airspeed_kt = 190.0F;
    entrees.gear_down_locked = false;
    entrees.on_ground = false;

    const Mode4aConditions conditions = mod11::evaluate_conditions(entrees);
    // Les conditions sont "STRICTEMENT inferieur" : a la valeur exacte du
    // seuil, elles sont FAUSSES.
    CHECK_FALSE(conditions.altitude_below_limit);
    CHECK_FALSE(conditions.airspeed_below_limit);
    CHECK(conditions.gear_not_down);
    CHECK(conditions.airborne);
}

TEST_REQ(Conditions, juste_sous_les_seuils, "LLR-GPWS-010") {
    Mode4aInputs entrees;
    entrees.radio_altitude_ft = 499.99F;
    entrees.airspeed_kt = 189.99F;
    const Mode4aConditions conditions = mod11::evaluate_conditions(entrees);
    CHECK(conditions.altitude_below_limit);
    CHECK(conditions.airspeed_below_limit);
}

TEST_REQ(Conditions, train_et_sol, "LLR-GPWS-010") {
    Mode4aInputs entrees;
    entrees.gear_down_locked = true;
    entrees.on_ground = true;
    const Mode4aConditions conditions = mod11::evaluate_conditions(entrees);
    CHECK_FALSE(conditions.gear_not_down);
    CHECK_FALSE(conditions.airborne);
}

// =============================================================================
//  2. La decision mode 4A
// =============================================================================
TEST_REQ(Mode4a, alerte_si_les_quatre_conditions, "LLR-GPWS-020") {
    CHECK(mod11::mode4a_decision(true, true, true, true));
}

TEST_REQ(Mode4a, aucune_alerte_si_une_condition_manque, "LLR-GPWS-020") {
    CHECK_FALSE(mod11::mode4a_decision(false, true, true, true));
    CHECK_FALSE(mod11::mode4a_decision(true, false, true, true));
    CHECK_FALSE(mod11::mode4a_decision(true, true, false, true));
    CHECK_FALSE(mod11::mode4a_decision(true, true, true, false));
}

TEST_REQ(Integration, chaine_complete, "LLR-GPWS-021") {
    Mode4aInputs approche_dangereuse;
    approche_dangereuse.radio_altitude_ft = 300.0F;
    approche_dangereuse.airspeed_kt = 150.0F;
    approche_dangereuse.gear_down_locked = false;
    approche_dangereuse.on_ground = false;
    CHECK(mod11::mode4a_alert(approche_dangereuse));

    Mode4aInputs approche_normale = approche_dangereuse;
    approche_normale.gear_down_locked = true;
    CHECK_FALSE(mod11::mode4a_alert(approche_normale));

    Mode4aInputs au_sol = approche_dangereuse;
    au_sol.on_ground = true;
    CHECK_FALSE(mod11::mode4a_alert(au_sol));
}

// =============================================================================
//  3. MC/DC : LE coeur du module
// =============================================================================

TEST_REQ(Mcdc, couverture_de_decision_ne_suffit_pas, "LLR-GPWS-050") {
    // DEUX tests suffisent a atteindre 100 % de couverture de DECISION :
    // la decision prend ses deux issues. Beaucoup d'equipes s'arretent la...
    // et passent a cote de trois conditions sur quatre.
    DecisionRecorder enregistreur(4U);
    executer_mode4a(enregistreur, true, true, true, true);    // -> vrai
    executer_mode4a(enregistreur, false, false, false, false); // -> faux

    const McdcReport rapport = mod11::analyze_mcdc(enregistreur);

    // Critere 3 satisfait : les deux issues sont vues.
    CHECK(rapport.outcome_true_seen);
    CHECK(rapport.outcome_false_seen);
    // Critere 2 satisfait : chaque condition a pris ses deux valeurs.
    for (usize index = 0U; index < 4U; ++index) {
        CHECK(rapport.condition_both_values[index]);
    }
    // MAIS le critere 4 (independance) n'est satisfait pour AUCUNE condition :
    // les deux evaluations different sur les quatre conditions a la fois.
    CHECK_EQ(rapport.covered_count(), usize{0});
    CHECK_FALSE(rapport.is_complete());
}

TEST_REQ(Mcdc, mode4a_couverture_complete, "LLR-GPWS-020,LLR-GPWS-050") {
    // Jeu MINIMAL pour une conjonction de 4 conditions : N + 1 = 5 tests.
    //
    //   E0  T T T T  -> VRAI    (le cas de reference)
    //   E1  F T T T  -> faux    paire d'independance de C1 avec E0
    //   E2  T F T T  -> faux    paire de C2
    //   E3  T T F T  -> faux    paire de C3
    //   E4  T T T F  -> faux    paire de C4
    //
    // 5 tests au lieu de 16 : c'est tout l'interet de MC/DC.
    DecisionRecorder enregistreur(4U);
    executer_mode4a(enregistreur, true, true, true, true);
    executer_mode4a(enregistreur, false, true, true, true);
    executer_mode4a(enregistreur, true, false, true, true);
    executer_mode4a(enregistreur, true, true, false, true);
    executer_mode4a(enregistreur, true, true, true, false);

    const McdcReport rapport = mod11::analyze_mcdc(enregistreur);

    CHECK_EQ(rapport.covered_count(), usize{4});
    CHECK(rapport.is_complete());

    // Chaque paire d'independance implique bien l'evaluation de reference E0.
    for (usize condition = 0U; condition < 4U; ++condition) {
        CHECK(rapport.condition_covered[condition]);
        CHECK_EQ(rapport.pair_first[condition], usize{0});
        CHECK_EQ(rapport.pair_second[condition], condition + 1U);
    }
}

TEST_REQ(Mcdc, mode4a_jeu_incomplet_detecte, "LLR-GPWS-050") {
    // Un jeu qui oublie la condition C4 : l'analyseur doit le dire.
    DecisionRecorder enregistreur(4U);
    executer_mode4a(enregistreur, true, true, true, true);
    executer_mode4a(enregistreur, false, true, true, true);
    executer_mode4a(enregistreur, true, false, true, true);
    executer_mode4a(enregistreur, true, true, false, true);

    const McdcReport rapport = mod11::analyze_mcdc(enregistreur);
    CHECK_EQ(rapport.covered_count(), usize{3});
    CHECK_FALSE(rapport.condition_covered[3]);
    CHECK_FALSE(rapport.condition_both_values[3]);  // C4 n'a jamais valu faux
    CHECK_FALSE(rapport.is_complete());
}

TEST_REQ(Mcdc, inhibition_couverture_complete, "LLR-GPWS-030,LLR-GPWS-050") {
    // Decision MIXTE : A OU (B ET C). Le jeu minimal n'a rien d'evident.
    //
    //   E0  F F T  -> faux
    //   E1  V F T  -> VRAI   paire de A avec E0 (seul A change)
    //   E2  F V T  -> VRAI   paire de B avec E0 (seul B change)
    //   E3  F V F  -> faux   paire de C avec E2 (seul C change)
    //
    // 4 tests pour 3 conditions : N + 1, l'optimum theorique.
    DecisionRecorder enregistreur(3U);
    executer_inhibition(enregistreur, false, false, true);
    executer_inhibition(enregistreur, true, false, true);
    executer_inhibition(enregistreur, false, true, true);
    executer_inhibition(enregistreur, false, true, false);

    const McdcReport rapport = mod11::analyze_mcdc(enregistreur);
    CHECK_EQ(rapport.covered_count(), usize{3});
    CHECK(rapport.is_complete());
}

TEST_REQ(Mcdc, inhibition_exhaustif_est_aussi_complet, "LLR-GPWS-030") {
    // Les 8 combinaisons : evidemment complet, mais 8 tests au lieu de 4.
    // Sur une decision a 16 conditions, ce serait 65 536 tests contre 17.
    DecisionRecorder enregistreur(3U);
    for (avio::u32 masque = 0U; masque < 8U; ++masque) {
        executer_inhibition(enregistreur, (masque & 4U) != 0U, (masque & 2U) != 0U,
                            (masque & 1U) != 0U);
    }
    const McdcReport rapport = mod11::analyze_mcdc(enregistreur);
    CHECK(rapport.is_complete());
    CHECK_EQ(enregistreur.size(), usize{8});
}

TEST_REQ(Mcdc, table_de_verite_inhibition, "LLR-GPWS-030") {
    // Verification exhaustive de la table de verite : A OU (B ET C).
    CHECK_FALSE(mod11::inhibition_decision(false, false, false));
    CHECK_FALSE(mod11::inhibition_decision(false, false, true));
    CHECK_FALSE(mod11::inhibition_decision(false, true, false));
    CHECK(mod11::inhibition_decision(false, true, true));
    CHECK(mod11::inhibition_decision(true, false, false));
    CHECK(mod11::inhibition_decision(true, false, true));
    CHECK(mod11::inhibition_decision(true, true, false));
    CHECK(mod11::inhibition_decision(true, true, true));
}

// =============================================================================
//  4. Alerte effective
// =============================================================================
TEST_REQ(Effective, alerte_emise_si_non_inhibee, "LLR-GPWS-040") {
    Mode4aInputs entrees;
    entrees.radio_altitude_ft = 300.0F;
    entrees.airspeed_kt = 150.0F;
    entrees.gear_down_locked = false;
    entrees.on_ground = false;

    CHECK(mod11::effective_alert(entrees, false, false, false));
    CHECK_FALSE(mod11::effective_alert(entrees, true, false, false));   // mode test
    CHECK_FALSE(mod11::effective_alert(entrees, false, true, true));    // approche stabilisee
    CHECK(mod11::effective_alert(entrees, false, true, false));         // config seule : pas inhibe
}

TEST_REQ(Effective, pas_d_alerte_sans_condition, "LLR-GPWS-040") {
    Mode4aInputs entrees;
    entrees.radio_altitude_ft = 3000.0F;  // trop haut
    entrees.airspeed_kt = 150.0F;
    CHECK_FALSE(mod11::effective_alert(entrees, false, false, false));
}

// =============================================================================
//  5. L'analyseur lui-meme (DO-330 : verifier l'outil de verification)
// =============================================================================
TEST_REQ(Analyseur, robustesse_enregistreur, "LLR-GPWS-050") {
    DecisionRecorder enregistreur(3U);
    const bool bonnes[3] = {true, false, true};
    const bool mauvaises[2] = {true, false};

    CHECK(enregistreur.record(bonnes, 3U, true));
    CHECK_FALSE(enregistreur.record(mauvaises, 2U, true));  // mauvais nombre
    CHECK_FALSE(enregistreur.record(nullptr, 3U, true));    // pointeur nul
    CHECK_EQ(enregistreur.size(), usize{1});

    enregistreur.reset();
    CHECK_EQ(enregistreur.size(), usize{0});
}

TEST_REQ(Analyseur, capacite_bornee, "LLR-GPWS-050") {
    DecisionRecorder enregistreur(1U);
    const bool condition[1] = {true};
    for (usize index = 0U; index < mod11::kMaxEvaluations; ++index) {
        CHECK(enregistreur.record(condition, 1U, true));
    }
    // Au-dela : refus, jamais de debordement.
    CHECK_FALSE(enregistreur.record(condition, 1U, true));
    CHECK_EQ(enregistreur.size(), mod11::kMaxEvaluations);
}

TEST_REQ(Analyseur, jeu_vide, "LLR-GPWS-050") {
    const DecisionRecorder enregistreur(4U);
    const McdcReport rapport = mod11::analyze_mcdc(enregistreur);
    CHECK_FALSE(rapport.outcome_true_seen);
    CHECK_FALSE(rapport.outcome_false_seen);
    CHECK_EQ(rapport.covered_count(), usize{0});
    CHECK_FALSE(rapport.is_complete());
}
