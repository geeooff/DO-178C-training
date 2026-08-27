// =============================================================================
//  Module 10 -- campagne de test complete d'une machine a etats.
//
//  Les cas sont regroupes PAR TECHNIQUE DE CONCEPTION, pas par ordre
//  d'ecriture du code. C'est ce regroupement que l'on presente en revue de
//  test : il montre que la couverture des exigences a ete RAISONNEE, et non
//  obtenue au jugé.
//
//    1. classes d'equivalence
//    2. analyse des valeurs limites
//    3. couverture des ETATS
//    4. couverture des TRANSITIONS
//    5. sequences realistes
//    6. robustesse
// =============================================================================
#include <avio/types.hpp>
#include <limits>
#include <microtest/microtest.hpp>

#include "mod10/alert_monitor.hpp"

using avio::f32;
using avio::u16;
using avio::u32;
using avio::usize;
using mod10::AlertConfig;
using mod10::AlertMonitor;
using mod10::AlertState;

namespace {

constexpr f32 kNaN = std::numeric_limits<f32>::quiet_NaN();
constexpr f32 kInf = std::numeric_limits<f32>::infinity();

/// Configuration de reference : seuil 100, retombee 90, 3 cycles de
/// confirmation, 2 cycles de retombee.
AlertConfig configuration_reference() noexcept {
    AlertConfig config;
    config.raise_threshold = 100.0F;
    config.clear_threshold = 90.0F;
    config.confirm_cycles = 3U;
    config.clear_cycles = 2U;
    return config;
}

AlertMonitor reference_monitor() noexcept {
    AlertMonitor monitor;
    (void)AlertMonitor::create(configuration_reference(), monitor);
    return monitor;
}

/// Amene le moniteur a l'etat Active en appliquant la sequence minimale.
void drive_to_active(AlertMonitor& monitor) noexcept {
    for (u16 cycle = 0U; cycle < monitor.config().confirm_cycles; ++cycle) {
        (void)monitor.update(150.0F);
    }
}

}  // namespace

// =============================================================================
//  0. Configuration et initialisation
// =============================================================================
TEST_REQ(Configuration, valid_configuration_accepted, "LLR-ALERT-010") {
    AlertMonitor monitor;
    CHECK(AlertMonitor::create(configuration_reference(), monitor));
}

TEST_REQ(Configuration, hysteresis_mandatory, "LLR-ALERT-010") {
    AlertMonitor monitor;

    // clear == raise : pas d'hysteresis -> battement garanti.
    AlertConfig equal = configuration_reference();
    equal.clear_threshold = equal.raise_threshold;
    CHECK_FALSE(AlertMonitor::create(equal, monitor));

    // clear > raise : incoherent.
    AlertConfig inverse = configuration_reference();
    inverse.clear_threshold = 150.0F;
    CHECK_FALSE(AlertMonitor::create(inverse, monitor));
}

TEST_REQ(Configuration, non_zero_counters, "LLR-ALERT-010") {
    AlertMonitor monitor;

    AlertConfig without_confirmation = configuration_reference();
    without_confirmation.confirm_cycles = 0U;
    CHECK_FALSE(AlertMonitor::create(without_confirmation, monitor));

    AlertConfig without_decay = configuration_reference();
    without_decay.clear_cycles = 0U;
    CHECK_FALSE(AlertMonitor::create(without_decay, monitor));
}

TEST_REQ(Configuration, non_finite_thresholds_rejected, "LLR-ALERT-010") {
    AlertMonitor monitor;

    AlertConfig nan_climb = configuration_reference();
    nan_climb.raise_threshold = kNaN;
    CHECK_FALSE(AlertMonitor::create(nan_climb, monitor));

    AlertConfig inf_decay = configuration_reference();
    inf_decay.clear_threshold = -kInf;
    CHECK_FALSE(AlertMonitor::create(inf_decay, monitor));
}

TEST_REQ(Initialization, start_state, "LLR-ALERT-011") {
    const AlertMonitor monitor = reference_monitor();
    CHECK_EQ(monitor.state(), AlertState::Inactive);
    CHECK_FALSE(monitor.is_raised());
    CHECK_EQ(monitor.confirm_progress(), u16{0});
    CHECK_EQ(monitor.activation_count(), u32{0});
    CHECK_EQ(monitor.rejected_samples(), u32{0});
}

TEST_REQ(Initialization, first_sample_does_not_raise_alert, "LLR-ALERT-011") {
    // HLR-ALERT-005 : meme un echantillon tres au-dessus du seuil ne doit pas
    // lever l'alerte au premier cycle (confirm_cycles = 3).
    AlertMonitor monitor = reference_monitor();
    CHECK_EQ(monitor.update(1.0e6F), AlertState::Pending);
    CHECK_FALSE(monitor.is_raised());
}

// =============================================================================
//  1. CLASSES D'EQUIVALENCE
//
//  Le domaine d'entree se partitionne en trois classes, chacune produisant un
//  comportement qualitativement different. Un representant par classe suffit :
//  tester 150 puis 200 n'apporte rien de plus que tester 150 seul.
//
//    C1 : v > 100          -> contribue a la montee
//    C2 : 90 <= v <= 100   -> zone morte (hysteresis) : ne contribue a rien
//    C3 : v < 90           -> contribue a la retombee
// =============================================================================
TEST_REQ(Equivalence, classifies_above_threshold, "LLR-ALERT-020") {
    AlertMonitor monitor = reference_monitor();
    CHECK_EQ(monitor.update(150.0F), AlertState::Pending);
    CHECK_EQ(monitor.confirm_progress(), u16{1});
}

TEST_REQ(Equivalence, classifies_dead_band, "LLR-ALERT-020") {
    // LA classe la plus interessante : entre les deux seuils, RIEN ne doit
    // bouger. C'est exactement ce que l'hysteresis doit produire.
    AlertMonitor monitor = reference_monitor();
    CHECK_EQ(monitor.update(95.0F), AlertState::Inactive);
    CHECK_EQ(monitor.confirm_progress(), u16{0});

    drive_to_active(monitor);
    CHECK_EQ(monitor.state(), AlertState::Active);
    CHECK_EQ(monitor.update(95.0F), AlertState::Active);  // toujours active
}

TEST_REQ(Equivalence, classifies_below_clear_threshold, "LLR-ALERT-030") {
    AlertMonitor monitor = reference_monitor();
    drive_to_active(monitor);
    CHECK_EQ(monitor.update(50.0F), AlertState::Clearing);
    CHECK_EQ(monitor.confirm_progress(), u16{1});
}

// =============================================================================
//  2. ANALYSE DES VALEURS LIMITES
//
//  Les exigences disent "STRICTEMENT superieur" et "STRICTEMENT inferieur".
//  Un `>` ecrit `>=` par erreur est le defaut le plus frequent du metier, et
//  il ne se voit QUE sur la valeur exacte du seuil.
// =============================================================================
TEST_REQ(Limits, exact_raise_threshold_does_not_trigger, "LLR-ALERT-020") {
    AlertMonitor monitor = reference_monitor();
    // Exactement 100,0 : la condition est "> 100", donc rien ne se passe.
    CHECK_EQ(monitor.update(100.0F), AlertState::Inactive);
    CHECK_EQ(monitor.confirm_progress(), u16{0});
}

TEST_REQ(Limits, just_above_threshold_triggers, "LLR-ALERT-020") {
    AlertMonitor monitor = reference_monitor();
    CHECK_EQ(monitor.update(100.001F), AlertState::Pending);
}

TEST_REQ(Limits, exact_clear_threshold_does_not_clear, "LLR-ALERT-030") {
    AlertMonitor monitor = reference_monitor();
    drive_to_active(monitor);
    // Exactement 90,0 : la condition est "< 90", donc l'alerte reste active.
    CHECK_EQ(monitor.update(90.0F), AlertState::Active);
}

TEST_REQ(Limits, just_below_threshold_clears, "LLR-ALERT-030") {
    AlertMonitor monitor = reference_monitor();
    drive_to_active(monitor);
    CHECK_EQ(monitor.update(89.999F), AlertState::Clearing);
}

TEST_REQ(Limits, single_cycle_confirmation, "LLR-ALERT-020") {
    // Valeur limite sur un PARAMETRE, pas sur une entree : confirm_cycles = 1
    // court-circuite l'etat Pending. C'est un chemin de code distinct.
    AlertConfig config = configuration_reference();
    config.confirm_cycles = 1U;
    AlertMonitor monitor;
    REQUIRE(AlertMonitor::create(config, monitor));

    CHECK_EQ(monitor.update(150.0F), AlertState::Active);
    CHECK_EQ(monitor.activation_count(), u32{1});
}

TEST_REQ(Limits, single_cycle_decay, "LLR-ALERT-030") {
    AlertConfig config = configuration_reference();
    config.clear_cycles = 1U;
    AlertMonitor monitor;
    REQUIRE(AlertMonitor::create(config, monitor));

    drive_to_active(monitor);
    CHECK_EQ(monitor.update(50.0F), AlertState::Inactive);
}

// =============================================================================
//  3. COUVERTURE DES ETATS
//
//  Les quatre etats doivent etre atteints. C'est le minimum absolu, et c'est
//  loin d'etre suffisant : voir la couverture des transitions ci-dessous.
// =============================================================================
TEST_REQ(States, all_four_states_are_reachable, "LLR-ALERT-020,LLR-ALERT-030") {
    AlertMonitor monitor = reference_monitor();
    CHECK_EQ(monitor.state(), AlertState::Inactive);

    CHECK_EQ(monitor.update(150.0F), AlertState::Pending);
    CHECK_EQ(monitor.update(150.0F), AlertState::Pending);
    CHECK_EQ(monitor.update(150.0F), AlertState::Active);
    CHECK_EQ(monitor.update(50.0F), AlertState::Clearing);
    CHECK_EQ(monitor.update(50.0F), AlertState::Inactive);
}

TEST_REQ(States, labels, "LLR-ALERT-011") {
    CHECK_EQ(mod10::state_name(AlertState::Inactive), "Inactive");
    CHECK_EQ(mod10::state_name(AlertState::Pending), "Pending");
    CHECK_EQ(mod10::state_name(AlertState::Active), "Active");
    CHECK_EQ(mod10::state_name(AlertState::Clearing), "Clearing");
    CHECK_EQ(mod10::state_name(static_cast<AlertState>(avio::u8{9U})), "Inconnu");
}

// =============================================================================
//  4. COUVERTURE DES TRANSITIONS
//
//  Six transitions nommees dans le SDD, plus les auto-transitions. Chacune est
//  un cas de test distinct. Atteindre les quatre etats SANS couvrir les
//  transitions laisse passer les defauts les plus courants : compteur non
//  reinitialise, transition inverse manquante.
// =============================================================================
TEST_REQ(Transitions, inactive_to_pending, "LLR-ALERT-020") {
    AlertMonitor monitor = reference_monitor();
    CHECK_EQ(monitor.update(150.0F), AlertState::Pending);
    CHECK_EQ(monitor.confirm_progress(), u16{1});
}

TEST_REQ(Transitions, pending_to_pending, "LLR-ALERT-021") {
    AlertMonitor monitor = reference_monitor();
    (void)monitor.update(150.0F);
    CHECK_EQ(monitor.update(150.0F), AlertState::Pending);
    CHECK_EQ(monitor.confirm_progress(), u16{2});
}

TEST_REQ(Transitions, pending_to_active, "LLR-ALERT-021") {
    AlertMonitor monitor = reference_monitor();
    (void)monitor.update(150.0F);
    (void)monitor.update(150.0F);
    CHECK_EQ(monitor.update(150.0F), AlertState::Active);
    CHECK_EQ(monitor.confirm_progress(), u16{0});
    CHECK_EQ(monitor.activation_count(), u32{1});
}

TEST_REQ(Transitions, pending_to_inactive_cancellation, "LLR-ALERT-022") {
    // LE test qui compte : la confirmation doit porter sur des cycles
    // CONSECUTIFS. Un compteur qui ne se remettrait pas a zero declencherait
    // l'alerte sur des depassements isoles cumules.
    AlertMonitor monitor = reference_monitor();
    (void)monitor.update(150.0F);
    (void)monitor.update(150.0F);
    CHECK_EQ(monitor.confirm_progress(), u16{2});

    CHECK_EQ(monitor.update(95.0F), AlertState::Inactive);
    CHECK_EQ(monitor.confirm_progress(), u16{0});
}

TEST_REQ(Transitions, active_to_clearing, "LLR-ALERT-030") {
    AlertMonitor monitor = reference_monitor();
    drive_to_active(monitor);
    CHECK_EQ(monitor.update(50.0F), AlertState::Clearing);
}

TEST_REQ(Transitions, clearing_to_inactive, "LLR-ALERT-031") {
    AlertMonitor monitor = reference_monitor();
    drive_to_active(monitor);
    (void)monitor.update(50.0F);
    CHECK_EQ(monitor.update(50.0F), AlertState::Inactive);
    CHECK_EQ(monitor.confirm_progress(), u16{0});
}

TEST_REQ(Transitions, clearing_to_active_cancellation, "LLR-ALERT-032") {
    AlertMonitor monitor = reference_monitor();
    drive_to_active(monitor);
    (void)monitor.update(50.0F);
    CHECK_EQ(monitor.state(), AlertState::Clearing);

    CHECK_EQ(monitor.update(95.0F), AlertState::Active);
    CHECK_EQ(monitor.confirm_progress(), u16{0});
}

TEST_REQ(Transitions, inactive_stays_inactive, "LLR-ALERT-020") {
    AlertMonitor monitor = reference_monitor();
    CHECK_EQ(monitor.update(50.0F), AlertState::Inactive);
    CHECK_EQ(monitor.update(95.0F), AlertState::Inactive);
}

TEST_REQ(Transitions, active_stays_active, "LLR-ALERT-030") {
    AlertMonitor monitor = reference_monitor();
    drive_to_active(monitor);
    CHECK_EQ(monitor.update(150.0F), AlertState::Active);
    CHECK_EQ(monitor.update(95.0F), AlertState::Active);
}

// =============================================================================
//  5. SEQUENCES REALISTES
//
//  Les tests unitaires par transition ne suffisent pas : les defauts se
//  cachent dans les enchainements. Ces sequences reproduisent des profils
//  physiques plausibles.
// =============================================================================
TEST_REQ(Sequences, sensor_noise_does_not_raise_alert, "LLR-ALERT-022") {
    // Profil : la valeur depasse le seuil un cycle sur deux (bruit). Sans
    // l'anti-rebond, l'alerte se leverait. C'est LE scenario que
    // confirm_cycles doit filtrer.
    AlertMonitor monitor = reference_monitor();
    const f32 profile[10] = {150.0F, 95.0F,  150.0F, 95.0F,  150.0F,
                             95.0F,  150.0F, 95.0F,  150.0F, 95.0F};
    for (usize index = 0U; index < 10U; ++index) {
        (void)monitor.update(profile[index]);
    }
    CHECK_FALSE(monitor.is_raised());
    CHECK_EQ(monitor.activation_count(), u32{0});
}

TEST_REQ(Sequences, oscillation_in_dead_band, "LLR-ALERT-032,LLR-ALERT-040") {
    // Alerte levee, puis la valeur oscille autour du seuil de retombee.
    // L'hysteresis doit maintenir l'alerte, et le compteur d'activations ne
    // doit PAS augmenter : sinon la maintenance verrait des dizaines
    // d'activations la ou il n'y en a eu qu'une.
    AlertMonitor monitor = reference_monitor();
    drive_to_active(monitor);
    CHECK_EQ(monitor.activation_count(), u32{1});

    for (usize index = 0U; index < 20U; ++index) {
        (void)monitor.update(((index % 2U) == 0U) ? 85.0F : 95.0F);
    }
    CHECK(monitor.is_raised());
    CHECK_EQ(monitor.activation_count(), u32{1});
}

TEST_REQ(Sequences, full_cycle_then_reactivation, "LLR-ALERT-040") {
    AlertMonitor monitor = reference_monitor();

    drive_to_active(monitor);
    (void)monitor.update(50.0F);
    (void)monitor.update(50.0F);
    REQUIRE_EQ(monitor.state(), AlertState::Inactive);
    CHECK_EQ(monitor.activation_count(), u32{1});

    drive_to_active(monitor);
    CHECK(monitor.is_raised());
    CHECK_EQ(monitor.activation_count(), u32{2});
}

TEST_REQ(Sequences, slow_climb_then_slow_descent, "LLR-ALERT-020,LLR-ALERT-030") {
    // Profil physique realiste : une rampe. L'alerte doit se lever 3 cycles
    // apres le franchissement du seuil, et retomber 2 cycles apres le
    // franchissement du seuil bas.
    AlertMonitor monitor = reference_monitor();

    // Compteur ENTIER, valeur calculee : un compteur flottant accumule
    // l'erreur d'arrondi et rend le nombre d'iterations dependant de la cible.
    // Regle cert-flp30-c, verifiee par clang-tidy.
    for (u16 pas = 0U; pas <= 6U; ++pas) {
        (void)monitor.update(80.0F + (static_cast<f32>(pas) * 5.0F));
    }
    // 105 et 110 depassent le seuil : 2 cycles seulement, pas encore active.
    CHECK_EQ(monitor.state(), AlertState::Pending);
    CHECK_EQ(monitor.update(115.0F), AlertState::Active);

    for (u16 pas = 0U; pas <= 6U; ++pas) {
        (void)monitor.update(110.0F - (static_cast<f32>(pas) * 5.0F));
    }
    CHECK_EQ(monitor.state(), AlertState::Inactive);
}

// =============================================================================
//  6. ROBUSTESSE
// =============================================================================
TEST_REQ(Robustness, non_finite_sample_ignored, "LLR-ALERT-050") {
    AlertMonitor monitor = reference_monitor();
    (void)monitor.update(150.0F);
    (void)monitor.update(150.0F);
    const u16 progress = monitor.confirm_progress();

    // Un capteur en panne ne doit ni lever ni effacer l'alerte.
    CHECK_EQ(monitor.update(kNaN), AlertState::Pending);
    CHECK_EQ(monitor.confirm_progress(), progress);
    CHECK_EQ(monitor.update(kInf), AlertState::Pending);
    CHECK_EQ(monitor.update(-kInf), AlertState::Pending);
    CHECK_EQ(monitor.confirm_progress(), progress);
    CHECK_EQ(monitor.activation_count(), u32{0});
}

TEST_REQ(Robustness, non_finite_during_active_alert, "LLR-ALERT-050") {
    AlertMonitor monitor = reference_monitor();
    drive_to_active(monitor);
    CHECK_EQ(monitor.update(kNaN), AlertState::Active);
    CHECK(monitor.is_raised());
}

TEST_REQ(Robustness, rejection_counter, "LLR-ALERT-051") {
    // Sans ce compteur, un capteur qui n'emet que des NaN laisserait l'alerte
    // eternellement inactive SANS que personne ne s'en apercoive. Le rejet
    // silencieux devient observable.
    AlertMonitor monitor = reference_monitor();
    CHECK_EQ(monitor.rejected_samples(), u32{0});

    (void)monitor.update(kNaN);
    (void)monitor.update(kInf);
    (void)monitor.update(150.0F);
    (void)monitor.update(-kInf);

    CHECK_EQ(monitor.rejected_samples(), u32{3});
}

// =============================================================================
//  7. PRESENTATION DE L'ALERTE
//
//  Distinction subtile mais essentielle : l'ETAT interne de la machine et ce
//  qui est PRESENTE a l'equipage ne sont pas la meme chose.
// =============================================================================
TEST_REQ(States, alert_presented_during_decay, "LLR-ALERT-033") {
    AlertMonitor monitor = reference_monitor();

    // Inactive et Pending : rien n'est presente.
    CHECK_FALSE(monitor.is_raised());
    (void)monitor.update(150.0F);
    REQUIRE_EQ(monitor.state(), AlertState::Pending);
    CHECK_FALSE(monitor.is_raised());

    // Active : l'alerte est presentee.
    (void)monitor.update(150.0F);
    (void)monitor.update(150.0F);
    REQUIRE_EQ(monitor.state(), AlertState::Active);
    CHECK(monitor.is_raised());

    // Clearing : l'alerte reste PRESENTEE tant que la retombee n'est pas
    // confirmee. Sans cela, clear_cycles ne servirait a rien : l'alerte
    // disparaitrait des le premier echantillon sous le seuil, et l'anti-rebond
    // ne jouerait que dans un sens.
    (void)monitor.update(50.0F);
    REQUIRE_EQ(monitor.state(), AlertState::Clearing);
    CHECK(monitor.is_raised());

    // Inactive : l'alerte disparait.
    (void)monitor.update(50.0F);
    REQUIRE_EQ(monitor.state(), AlertState::Inactive);
    CHECK_FALSE(monitor.is_raised());
}

TEST_REQ(States, cancelled_decay_holds_alert, "LLR-ALERT-033,LLR-ALERT-032") {
    // Le scenario reel : la valeur passe brievement sous le seuil de retombee
    // puis remonte. L'alerte ne doit jamais avoir clignote.
    AlertMonitor monitor = reference_monitor();
    drive_to_active(monitor);

    for (usize index = 0U; index < 30U; ++index) {
        (void)monitor.update(((index % 2U) == 0U) ? 50.0F : 150.0F);
        CHECK(monitor.is_raised());
    }
}
