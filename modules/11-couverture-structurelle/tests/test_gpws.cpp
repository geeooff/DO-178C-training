#include <avio/types.hpp>
#include <microtest/microtest.hpp>

#include "mod11/gpws.hpp"
#include "mod11/mcdc.hpp"

using avio::usize;
using mod11::DecisionRecorder;
using mod11::McdcReport;
using mod11::Mode4aConditions;
using mod11::Mode4aInputs;

namespace {

/// Applique un vecteur de conditions a la decision mode 4A et l'enregistre.
void run_mode4a(DecisionRecorder& recorder, bool c1, bool c2, bool c3, bool c4) noexcept {
    const bool conditions[4] = {c1, c2, c3, c4};
    const bool issue = mod11::mode4a_decision(c1, c2, c3, c4);
    (void)recorder.record(conditions, 4U, issue);
}

void run_inhibition(DecisionRecorder& recorder, bool a, bool b, bool c) noexcept {
    const bool conditions[3] = {a, b, c};
    const bool issue = mod11::inhibition_decision(a, b, c);
    (void)recorder.record(conditions, 3U, issue);
}

}  // namespace

// =============================================================================
//  1. Evaluation des conditions (valeurs limites)
// =============================================================================
TEST_REQ(Conditions, exact_thresholds, "LLR-GPWS-010") {
    Mode4aInputs inputs;
    inputs.radio_altitude_ft = 500.0F;
    inputs.airspeed_kt = 190.0F;
    inputs.gear_down_locked = false;
    inputs.on_ground = false;

    const Mode4aConditions conditions = mod11::evaluate_conditions(inputs);
    // Les conditions sont "STRICTEMENT inferieur" : a la valeur exacte du
    // seuil, elles sont FAUSSES.
    CHECK_FALSE(conditions.altitude_below_limit);
    CHECK_FALSE(conditions.airspeed_below_limit);
    CHECK(conditions.gear_not_down);
    CHECK(conditions.airborne);
}

TEST_REQ(Conditions, just_below_thresholds, "LLR-GPWS-010") {
    Mode4aInputs inputs;
    inputs.radio_altitude_ft = 499.99F;
    inputs.airspeed_kt = 189.99F;
    const Mode4aConditions conditions = mod11::evaluate_conditions(inputs);
    CHECK(conditions.altitude_below_limit);
    CHECK(conditions.airspeed_below_limit);
}

TEST_REQ(Conditions, gear_and_ground, "LLR-GPWS-010") {
    Mode4aInputs inputs;
    inputs.gear_down_locked = true;
    inputs.on_ground = true;
    const Mode4aConditions conditions = mod11::evaluate_conditions(inputs);
    CHECK_FALSE(conditions.gear_not_down);
    CHECK_FALSE(conditions.airborne);
}

// =============================================================================
//  2. La decision mode 4A
// =============================================================================
TEST_REQ(Mode4a, alert_when_all_four_conditions, "LLR-GPWS-020") {
    CHECK(mod11::mode4a_decision(true, true, true, true));
}

TEST_REQ(Mode4a, no_alert_if_one_condition_missing, "LLR-GPWS-020") {
    CHECK_FALSE(mod11::mode4a_decision(false, true, true, true));
    CHECK_FALSE(mod11::mode4a_decision(true, false, true, true));
    CHECK_FALSE(mod11::mode4a_decision(true, true, false, true));
    CHECK_FALSE(mod11::mode4a_decision(true, true, true, false));
}

TEST_REQ(Integration, complete_chain, "LLR-GPWS-021") {
    Mode4aInputs hazardous_approach;
    hazardous_approach.radio_altitude_ft = 300.0F;
    hazardous_approach.airspeed_kt = 150.0F;
    hazardous_approach.gear_down_locked = false;
    hazardous_approach.on_ground = false;
    CHECK(mod11::mode4a_alert(hazardous_approach));

    Mode4aInputs nominal_approach = hazardous_approach;
    nominal_approach.gear_down_locked = true;
    CHECK_FALSE(mod11::mode4a_alert(nominal_approach));

    Mode4aInputs ground = hazardous_approach;
    ground.on_ground = true;
    CHECK_FALSE(mod11::mode4a_alert(ground));
}

// =============================================================================
//  3. MC/DC : LE coeur du module
// =============================================================================

TEST_REQ(Mcdc, decision_coverage_is_not_enough, "LLR-GPWS-050") {
    // DEUX tests suffisent a atteindre 100 % de couverture de DECISION :
    // la decision prend ses deux issues. Beaucoup d'equipes s'arretent la...
    // et passent a cote de trois conditions sur quatre.
    DecisionRecorder recorder(4U);
    run_mode4a(recorder, true, true, true, true);      // -> vrai
    run_mode4a(recorder, false, false, false, false);  // -> faux

    const McdcReport report = mod11::analyze_mcdc(recorder);

    // Critere 3 satisfait : les deux issues sont vues.
    CHECK(report.outcome_true_seen);
    CHECK(report.outcome_false_seen);
    // Critere 2 satisfait : chaque condition a pris ses deux valeurs.
    for (usize index = 0U; index < 4U; ++index) {
        CHECK(report.condition_both_values[index]);
    }
    // MAIS le critere 4 (independance) n'est satisfait pour AUCUNE condition :
    // les deux evaluations different sur les quatre conditions a la fois.
    CHECK_EQ(report.covered_count(), usize{0});
    CHECK_FALSE(report.is_complete());
}

TEST_REQ(Mcdc, mode4a_full_coverage, "LLR-GPWS-020,LLR-GPWS-050") {
    // Jeu MINIMAL pour une conjonction de 4 conditions : N + 1 = 5 tests.
    //
    //   E0  T T T T  -> VRAI    (le cas de reference)
    //   E1  F T T T  -> faux    paire d'independance de C1 avec E0
    //   E2  T F T T  -> faux    paire de C2
    //   E3  T T F T  -> faux    paire de C3
    //   E4  T T T F  -> faux    paire de C4
    //
    // 5 tests au lieu de 16 : c'est tout l'interet de MC/DC.
    DecisionRecorder recorder(4U);
    run_mode4a(recorder, true, true, true, true);
    run_mode4a(recorder, false, true, true, true);
    run_mode4a(recorder, true, false, true, true);
    run_mode4a(recorder, true, true, false, true);
    run_mode4a(recorder, true, true, true, false);

    const McdcReport report = mod11::analyze_mcdc(recorder);

    CHECK_EQ(report.covered_count(), usize{4});
    CHECK(report.is_complete());

    // Chaque paire d'independance implique bien l'evaluation de reference E0.
    for (usize condition = 0U; condition < 4U; ++condition) {
        CHECK(report.condition_covered[condition]);
        CHECK_EQ(report.pair_first[condition], usize{0});
        CHECK_EQ(report.pair_second[condition], condition + 1U);
    }
}

TEST_REQ(Mcdc, mode4a_incomplete_set_detected, "LLR-GPWS-050") {
    // Un jeu qui oublie la condition C4 : l'analyseur doit le dire.
    DecisionRecorder recorder(4U);
    run_mode4a(recorder, true, true, true, true);
    run_mode4a(recorder, false, true, true, true);
    run_mode4a(recorder, true, false, true, true);
    run_mode4a(recorder, true, true, false, true);

    const McdcReport report = mod11::analyze_mcdc(recorder);
    CHECK_EQ(report.covered_count(), usize{3});
    CHECK_FALSE(report.condition_covered[3]);
    CHECK_FALSE(report.condition_both_values[3]);  // C4 n'a jamais valu faux
    CHECK_FALSE(report.is_complete());
}

TEST_REQ(Mcdc, inhibition_full_coverage, "LLR-GPWS-030,LLR-GPWS-050") {
    // Decision MIXTE : A OU (B ET C). Le jeu minimal n'a rien d'evident.
    //
    //   E0  F F T  -> faux
    //   E1  V F T  -> VRAI   paire de A avec E0 (seul A change)
    //   E2  F V T  -> VRAI   paire de B avec E0 (seul B change)
    //   E3  F V F  -> faux   paire de C avec E2 (seul C change)
    //
    // 4 tests pour 3 conditions : N + 1, l'optimum theorique.
    DecisionRecorder recorder(3U);
    run_inhibition(recorder, false, false, true);
    run_inhibition(recorder, true, false, true);
    run_inhibition(recorder, false, true, true);
    run_inhibition(recorder, false, true, false);

    const McdcReport report = mod11::analyze_mcdc(recorder);
    CHECK_EQ(report.covered_count(), usize{3});
    CHECK(report.is_complete());
}

TEST_REQ(Mcdc, exhaustive_inhibition_is_also_complete, "LLR-GPWS-030") {
    // Les 8 combinaisons : evidemment complet, mais 8 tests au lieu de 4.
    // Sur une decision a 16 conditions, ce serait 65 536 tests contre 17.
    DecisionRecorder recorder(3U);
    for (avio::u32 mask = 0U; mask < 8U; ++mask) {
        run_inhibition(recorder, (mask & 4U) != 0U, (mask & 2U) != 0U, (mask & 1U) != 0U);
    }
    const McdcReport report = mod11::analyze_mcdc(recorder);
    CHECK(report.is_complete());
    CHECK_EQ(recorder.size(), usize{8});
}

TEST_REQ(Mcdc, inhibition_truth_table, "LLR-GPWS-030") {
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
TEST_REQ(Effective, alert_emitted_if_not_inhibited, "LLR-GPWS-040") {
    Mode4aInputs inputs;
    inputs.radio_altitude_ft = 300.0F;
    inputs.airspeed_kt = 150.0F;
    inputs.gear_down_locked = false;
    inputs.on_ground = false;

    CHECK(mod11::effective_alert(inputs, false, false, false));
    CHECK_FALSE(mod11::effective_alert(inputs, true, false, false));  // mode test
    CHECK_FALSE(mod11::effective_alert(inputs, false, true, true));   // approche stabilisee
    CHECK(mod11::effective_alert(inputs, false, true, false));        // config seule : pas inhibe
}

TEST_REQ(Effective, no_alert_without_condition, "LLR-GPWS-040") {
    Mode4aInputs inputs;
    inputs.radio_altitude_ft = 3000.0F;  // trop haut
    inputs.airspeed_kt = 150.0F;
    CHECK_FALSE(mod11::effective_alert(inputs, false, false, false));
}

// =============================================================================
//  5. L'analyseur lui-meme (DO-330 : verifier l'outil de verification)
// =============================================================================
TEST_REQ(Analyzer, robustness_recorder, "LLR-GPWS-050") {
    DecisionRecorder recorder(3U);
    const bool good[3] = {true, false, true};
    const bool bad[2] = {true, false};

    CHECK(recorder.record(good, 3U, true));
    CHECK_FALSE(recorder.record(bad, 2U, true));      // mauvais nombre
    CHECK_FALSE(recorder.record(nullptr, 3U, true));  // pointeur nul
    CHECK_EQ(recorder.size(), usize{1});

    recorder.reset();
    CHECK_EQ(recorder.size(), usize{0});
}

TEST_REQ(Analyzer, bounded_capacity, "LLR-GPWS-050") {
    DecisionRecorder recorder(1U);
    const bool condition[1] = {true};
    for (usize index = 0U; index < mod11::kMaxEvaluations; ++index) {
        CHECK(recorder.record(condition, 1U, true));
    }
    // Au-dela : refus, jamais de debordement.
    CHECK_FALSE(recorder.record(condition, 1U, true));
    CHECK_EQ(recorder.size(), mod11::kMaxEvaluations);
}

TEST_REQ(Analyzer, empty_set, "LLR-GPWS-050") {
    const DecisionRecorder recorder(4U);
    const McdcReport report = mod11::analyze_mcdc(recorder);
    CHECK_FALSE(report.outcome_true_seen);
    CHECK_FALSE(report.outcome_false_seen);
    CHECK_EQ(report.covered_count(), usize{0});
    CHECK_FALSE(report.is_complete());
}
