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
#include <microtest/microtest.hpp>

#include "mod10/alert_monitor.hpp"

#include <avio/types.hpp>

#include <limits>

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

AlertMonitor moniteur_reference() noexcept {
    AlertMonitor moniteur;
    (void)AlertMonitor::create(configuration_reference(), moniteur);
    return moniteur;
}

/// Amene le moniteur a l'etat Active en appliquant la sequence minimale.
void porter_a_active(AlertMonitor& moniteur) noexcept {
    for (u16 cycle = 0U; cycle < moniteur.config().confirm_cycles; ++cycle) {
        (void)moniteur.update(150.0F);
    }
}

}  // namespace

// =============================================================================
//  0. Configuration et initialisation
// =============================================================================
TEST_REQ(Configuration, configuration_valide_acceptee, "LLR-ALERT-010") {
    AlertMonitor moniteur;
    CHECK(AlertMonitor::create(configuration_reference(), moniteur));
}

TEST_REQ(Configuration, hysteresis_obligatoire, "LLR-ALERT-010") {
    AlertMonitor moniteur;

    // clear == raise : pas d'hysteresis -> battement garanti.
    AlertConfig egaux = configuration_reference();
    egaux.clear_threshold = egaux.raise_threshold;
    CHECK_FALSE(AlertMonitor::create(egaux, moniteur));

    // clear > raise : incoherent.
    AlertConfig inverses = configuration_reference();
    inverses.clear_threshold = 150.0F;
    CHECK_FALSE(AlertMonitor::create(inverses, moniteur));
}

TEST_REQ(Configuration, compteurs_non_nuls, "LLR-ALERT-010") {
    AlertMonitor moniteur;

    AlertConfig sans_confirmation = configuration_reference();
    sans_confirmation.confirm_cycles = 0U;
    CHECK_FALSE(AlertMonitor::create(sans_confirmation, moniteur));

    AlertConfig sans_retombee = configuration_reference();
    sans_retombee.clear_cycles = 0U;
    CHECK_FALSE(AlertMonitor::create(sans_retombee, moniteur));
}

TEST_REQ(Configuration, seuils_non_finis_refuses, "LLR-ALERT-010") {
    AlertMonitor moniteur;

    AlertConfig nan_montee = configuration_reference();
    nan_montee.raise_threshold = kNaN;
    CHECK_FALSE(AlertMonitor::create(nan_montee, moniteur));

    AlertConfig inf_retombee = configuration_reference();
    inf_retombee.clear_threshold = -kInf;
    CHECK_FALSE(AlertMonitor::create(inf_retombee, moniteur));
}

TEST_REQ(Initialisation, etat_de_depart, "LLR-ALERT-011") {
    const AlertMonitor moniteur = moniteur_reference();
    CHECK_EQ(moniteur.state(), AlertState::Inactive);
    CHECK_FALSE(moniteur.is_raised());
    CHECK_EQ(moniteur.confirm_progress(), u16{0});
    CHECK_EQ(moniteur.activation_count(), u32{0});
    CHECK_EQ(moniteur.rejected_samples(), u32{0});
}

TEST_REQ(Initialisation, premier_echantillon_ne_leve_pas_l_alerte, "LLR-ALERT-011") {
    // HLR-ALERT-005 : meme un echantillon tres au-dessus du seuil ne doit pas
    // lever l'alerte au premier cycle (confirm_cycles = 3).
    AlertMonitor moniteur = moniteur_reference();
    CHECK_EQ(moniteur.update(1.0e6F), AlertState::Pending);
    CHECK_FALSE(moniteur.is_raised());
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
TEST_REQ(Equivalence, classe_au_dessus_du_seuil, "LLR-ALERT-020") {
    AlertMonitor moniteur = moniteur_reference();
    CHECK_EQ(moniteur.update(150.0F), AlertState::Pending);
    CHECK_EQ(moniteur.confirm_progress(), u16{1});
}

TEST_REQ(Equivalence, classe_zone_morte, "LLR-ALERT-020") {
    // LA classe la plus interessante : entre les deux seuils, RIEN ne doit
    // bouger. C'est exactement ce que l'hysteresis doit produire.
    AlertMonitor moniteur = moniteur_reference();
    CHECK_EQ(moniteur.update(95.0F), AlertState::Inactive);
    CHECK_EQ(moniteur.confirm_progress(), u16{0});

    porter_a_active(moniteur);
    CHECK_EQ(moniteur.state(), AlertState::Active);
    CHECK_EQ(moniteur.update(95.0F), AlertState::Active);  // toujours active
}

TEST_REQ(Equivalence, classe_sous_le_seuil_de_retombee, "LLR-ALERT-030") {
    AlertMonitor moniteur = moniteur_reference();
    porter_a_active(moniteur);
    CHECK_EQ(moniteur.update(50.0F), AlertState::Clearing);
    CHECK_EQ(moniteur.confirm_progress(), u16{1});
}

// =============================================================================
//  2. ANALYSE DES VALEURS LIMITES
//
//  Les exigences disent "STRICTEMENT superieur" et "STRICTEMENT inferieur".
//  Un `>` ecrit `>=` par erreur est le defaut le plus frequent du metier, et
//  il ne se voit QUE sur la valeur exacte du seuil.
// =============================================================================
TEST_REQ(Limites, seuil_de_montee_exact_ne_declenche_pas, "LLR-ALERT-020") {
    AlertMonitor moniteur = moniteur_reference();
    // Exactement 100,0 : la condition est "> 100", donc rien ne se passe.
    CHECK_EQ(moniteur.update(100.0F), AlertState::Inactive);
    CHECK_EQ(moniteur.confirm_progress(), u16{0});
}

TEST_REQ(Limites, juste_au_dessus_du_seuil_declenche, "LLR-ALERT-020") {
    AlertMonitor moniteur = moniteur_reference();
    CHECK_EQ(moniteur.update(100.001F), AlertState::Pending);
}

TEST_REQ(Limites, seuil_de_retombee_exact_ne_retombe_pas, "LLR-ALERT-030") {
    AlertMonitor moniteur = moniteur_reference();
    porter_a_active(moniteur);
    // Exactement 90,0 : la condition est "< 90", donc l'alerte reste active.
    CHECK_EQ(moniteur.update(90.0F), AlertState::Active);
}

TEST_REQ(Limites, juste_sous_le_seuil_retombe, "LLR-ALERT-030") {
    AlertMonitor moniteur = moniteur_reference();
    porter_a_active(moniteur);
    CHECK_EQ(moniteur.update(89.999F), AlertState::Clearing);
}

TEST_REQ(Limites, confirmation_a_un_seul_cycle, "LLR-ALERT-020") {
    // Valeur limite sur un PARAMETRE, pas sur une entree : confirm_cycles = 1
    // court-circuite l'etat Pending. C'est un chemin de code distinct.
    AlertConfig config = configuration_reference();
    config.confirm_cycles = 1U;
    AlertMonitor moniteur;
    REQUIRE(AlertMonitor::create(config, moniteur));

    CHECK_EQ(moniteur.update(150.0F), AlertState::Active);
    CHECK_EQ(moniteur.activation_count(), u32{1});
}

TEST_REQ(Limites, retombee_a_un_seul_cycle, "LLR-ALERT-030") {
    AlertConfig config = configuration_reference();
    config.clear_cycles = 1U;
    AlertMonitor moniteur;
    REQUIRE(AlertMonitor::create(config, moniteur));

    porter_a_active(moniteur);
    CHECK_EQ(moniteur.update(50.0F), AlertState::Inactive);
}

// =============================================================================
//  3. COUVERTURE DES ETATS
//
//  Les quatre etats doivent etre atteints. C'est le minimum absolu, et c'est
//  loin d'etre suffisant : voir la couverture des transitions ci-dessous.
// =============================================================================
TEST_REQ(Etats, les_quatre_etats_sont_atteignables, "LLR-ALERT-020,LLR-ALERT-030") {
    AlertMonitor moniteur = moniteur_reference();
    CHECK_EQ(moniteur.state(), AlertState::Inactive);

    CHECK_EQ(moniteur.update(150.0F), AlertState::Pending);
    CHECK_EQ(moniteur.update(150.0F), AlertState::Pending);
    CHECK_EQ(moniteur.update(150.0F), AlertState::Active);
    CHECK_EQ(moniteur.update(50.0F), AlertState::Clearing);
    CHECK_EQ(moniteur.update(50.0F), AlertState::Inactive);
}

TEST_REQ(Etats, libelles, "LLR-ALERT-011") {
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
TEST_REQ(Transitions, inactive_vers_pending, "LLR-ALERT-020") {
    AlertMonitor moniteur = moniteur_reference();
    CHECK_EQ(moniteur.update(150.0F), AlertState::Pending);
    CHECK_EQ(moniteur.confirm_progress(), u16{1});
}

TEST_REQ(Transitions, pending_vers_pending, "LLR-ALERT-021") {
    AlertMonitor moniteur = moniteur_reference();
    (void)moniteur.update(150.0F);
    CHECK_EQ(moniteur.update(150.0F), AlertState::Pending);
    CHECK_EQ(moniteur.confirm_progress(), u16{2});
}

TEST_REQ(Transitions, pending_vers_active, "LLR-ALERT-021") {
    AlertMonitor moniteur = moniteur_reference();
    (void)moniteur.update(150.0F);
    (void)moniteur.update(150.0F);
    CHECK_EQ(moniteur.update(150.0F), AlertState::Active);
    CHECK_EQ(moniteur.confirm_progress(), u16{0});
    CHECK_EQ(moniteur.activation_count(), u32{1});
}

TEST_REQ(Transitions, pending_vers_inactive_annulation, "LLR-ALERT-022") {
    // LE test qui compte : la confirmation doit porter sur des cycles
    // CONSECUTIFS. Un compteur qui ne se remettrait pas a zero declencherait
    // l'alerte sur des depassements isoles cumules.
    AlertMonitor moniteur = moniteur_reference();
    (void)moniteur.update(150.0F);
    (void)moniteur.update(150.0F);
    CHECK_EQ(moniteur.confirm_progress(), u16{2});

    CHECK_EQ(moniteur.update(95.0F), AlertState::Inactive);
    CHECK_EQ(moniteur.confirm_progress(), u16{0});
}

TEST_REQ(Transitions, active_vers_clearing, "LLR-ALERT-030") {
    AlertMonitor moniteur = moniteur_reference();
    porter_a_active(moniteur);
    CHECK_EQ(moniteur.update(50.0F), AlertState::Clearing);
}

TEST_REQ(Transitions, clearing_vers_inactive, "LLR-ALERT-031") {
    AlertMonitor moniteur = moniteur_reference();
    porter_a_active(moniteur);
    (void)moniteur.update(50.0F);
    CHECK_EQ(moniteur.update(50.0F), AlertState::Inactive);
    CHECK_EQ(moniteur.confirm_progress(), u16{0});
}

TEST_REQ(Transitions, clearing_vers_active_annulation, "LLR-ALERT-032") {
    AlertMonitor moniteur = moniteur_reference();
    porter_a_active(moniteur);
    (void)moniteur.update(50.0F);
    CHECK_EQ(moniteur.state(), AlertState::Clearing);

    CHECK_EQ(moniteur.update(95.0F), AlertState::Active);
    CHECK_EQ(moniteur.confirm_progress(), u16{0});
}

TEST_REQ(Transitions, inactive_reste_inactive, "LLR-ALERT-020") {
    AlertMonitor moniteur = moniteur_reference();
    CHECK_EQ(moniteur.update(50.0F), AlertState::Inactive);
    CHECK_EQ(moniteur.update(95.0F), AlertState::Inactive);
}

TEST_REQ(Transitions, active_reste_active, "LLR-ALERT-030") {
    AlertMonitor moniteur = moniteur_reference();
    porter_a_active(moniteur);
    CHECK_EQ(moniteur.update(150.0F), AlertState::Active);
    CHECK_EQ(moniteur.update(95.0F), AlertState::Active);
}

// =============================================================================
//  5. SEQUENCES REALISTES
//
//  Les tests unitaires par transition ne suffisent pas : les defauts se
//  cachent dans les enchainements. Ces sequences reproduisent des profils
//  physiques plausibles.
// =============================================================================
TEST_REQ(Sequences, bruit_de_capteur_ne_leve_pas_l_alerte, "LLR-ALERT-022") {
    // Profil : la valeur depasse le seuil un cycle sur deux (bruit). Sans
    // l'anti-rebond, l'alerte se leverait. C'est LE scenario que
    // confirm_cycles doit filtrer.
    AlertMonitor moniteur = moniteur_reference();
    const f32 profil[10] = {150.0F, 95.0F, 150.0F, 95.0F, 150.0F,
                            95.0F,  150.0F, 95.0F, 150.0F, 95.0F};
    for (usize index = 0U; index < 10U; ++index) {
        (void)moniteur.update(profil[index]);
    }
    CHECK_FALSE(moniteur.is_raised());
    CHECK_EQ(moniteur.activation_count(), u32{0});
}

TEST_REQ(Sequences, oscillation_dans_la_zone_morte, "LLR-ALERT-032,LLR-ALERT-040") {
    // Alerte levee, puis la valeur oscille autour du seuil de retombee.
    // L'hysteresis doit maintenir l'alerte, et le compteur d'activations ne
    // doit PAS augmenter : sinon la maintenance verrait des dizaines
    // d'activations la ou il n'y en a eu qu'une.
    AlertMonitor moniteur = moniteur_reference();
    porter_a_active(moniteur);
    CHECK_EQ(moniteur.activation_count(), u32{1});

    for (usize index = 0U; index < 20U; ++index) {
        (void)moniteur.update(((index % 2U) == 0U) ? 85.0F : 95.0F);
    }
    CHECK(moniteur.is_raised());
    CHECK_EQ(moniteur.activation_count(), u32{1});
}

TEST_REQ(Sequences, cycle_complet_puis_reactivation, "LLR-ALERT-040") {
    AlertMonitor moniteur = moniteur_reference();

    porter_a_active(moniteur);
    (void)moniteur.update(50.0F);
    (void)moniteur.update(50.0F);
    REQUIRE_EQ(moniteur.state(), AlertState::Inactive);
    CHECK_EQ(moniteur.activation_count(), u32{1});

    porter_a_active(moniteur);
    CHECK(moniteur.is_raised());
    CHECK_EQ(moniteur.activation_count(), u32{2});
}

TEST_REQ(Sequences, montee_lente_puis_descente_lente, "LLR-ALERT-020,LLR-ALERT-030") {
    // Profil physique realiste : une rampe. L'alerte doit se lever 3 cycles
    // apres le franchissement du seuil, et retomber 2 cycles apres le
    // franchissement du seuil bas.
    AlertMonitor moniteur = moniteur_reference();

    // Compteur ENTIER, valeur calculee : un compteur flottant accumule
    // l'erreur d'arrondi et rend le nombre d'iterations dependant de la cible.
    // Regle cert-flp30-c, verifiee par clang-tidy.
    for (u16 pas = 0U; pas <= 6U; ++pas) {
        (void)moniteur.update(80.0F + (static_cast<f32>(pas) * 5.0F));
    }
    // 105 et 110 depassent le seuil : 2 cycles seulement, pas encore active.
    CHECK_EQ(moniteur.state(), AlertState::Pending);
    CHECK_EQ(moniteur.update(115.0F), AlertState::Active);

    for (u16 pas = 0U; pas <= 6U; ++pas) {
        (void)moniteur.update(110.0F - (static_cast<f32>(pas) * 5.0F));
    }
    CHECK_EQ(moniteur.state(), AlertState::Inactive);
}

// =============================================================================
//  6. ROBUSTESSE
// =============================================================================
TEST_REQ(Robustesse, echantillon_non_fini_ignore, "LLR-ALERT-050") {
    AlertMonitor moniteur = moniteur_reference();
    (void)moniteur.update(150.0F);
    (void)moniteur.update(150.0F);
    const u16 progression = moniteur.confirm_progress();

    // Un capteur en panne ne doit ni lever ni effacer l'alerte.
    CHECK_EQ(moniteur.update(kNaN), AlertState::Pending);
    CHECK_EQ(moniteur.confirm_progress(), progression);
    CHECK_EQ(moniteur.update(kInf), AlertState::Pending);
    CHECK_EQ(moniteur.update(-kInf), AlertState::Pending);
    CHECK_EQ(moniteur.confirm_progress(), progression);
    CHECK_EQ(moniteur.activation_count(), u32{0});
}

TEST_REQ(Robustesse, non_fini_pendant_alerte_active, "LLR-ALERT-050") {
    AlertMonitor moniteur = moniteur_reference();
    porter_a_active(moniteur);
    CHECK_EQ(moniteur.update(kNaN), AlertState::Active);
    CHECK(moniteur.is_raised());
}

TEST_REQ(Robustesse, compteur_de_rejets, "LLR-ALERT-051") {
    // Sans ce compteur, un capteur qui n'emet que des NaN laisserait l'alerte
    // eternellement inactive SANS que personne ne s'en apercoive. Le rejet
    // silencieux devient observable.
    AlertMonitor moniteur = moniteur_reference();
    CHECK_EQ(moniteur.rejected_samples(), u32{0});

    (void)moniteur.update(kNaN);
    (void)moniteur.update(kInf);
    (void)moniteur.update(150.0F);
    (void)moniteur.update(-kInf);

    CHECK_EQ(moniteur.rejected_samples(), u32{3});
}
