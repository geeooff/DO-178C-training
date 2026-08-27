// =============================================================================
//  Module 12 -- tests unitaires ET tests d'INTEGRATION.
//
//  La distinction est importante :
//    * les tests UNITAIRES verifient chaque composant isolement ;
//    * les tests d'INTEGRATION verifient leur ASSEMBLAGE, et c'est la seule
//      facon d'exercer les couplages.
//
//  Les suites `Couplage.*` demontrent l'objectif A-7.8 : elles prouvent, par
//  instrumentation, que toutes les interfaces declarees dans le SDD ont ete
//  exercees, et que les donnees echangees sont celles attendues.
// =============================================================================
#include <microtest/microtest.hpp>

#include "mod12/chain.hpp"
#include "mod12/coupling_trace.hpp"

#include <avio/types.hpp>

#include <limits>

using avio::f32;
using avio::i32;
using avio::u32;
using avio::usize;
using mod07::Status;
using mod12::Acquisition;
using mod12::CouplingTrace;
using mod12::CycleOutcome;
using mod12::Filter;
using mod12::Interface;

namespace {

constexpr f32 kNaN = std::numeric_limits<f32>::quiet_NaN();

}  // namespace

// =============================================================================
//  1. Tests UNITAIRES : chaque composant isolement
// =============================================================================
TEST_REQ(Acquisition, conversion_nominale, "LLR-CHAIN-010") {
    Acquisition acquisition;
    acquisition.set_raw(2000);
    const mod07::Result<f32> resultat = acquisition.read();
    REQUIRE(resultat.is_ok());
    CHECK_NEAR(static_cast<double>(resultat.value()), 500.0, 1e-3);
    CHECK_EQ(acquisition.read_count(), u32{1});
    CHECK_EQ(acquisition.reject_count(), u32{0});
}

TEST_REQ(Acquisition, bornes_du_convertisseur, "LLR-CHAIN-010") {
    Acquisition acquisition;

    acquisition.set_raw(Acquisition::kRawMin);
    CHECK(acquisition.read().is_ok());

    acquisition.set_raw(Acquisition::kRawMax);
    const mod07::Result<f32> plein_echelle = acquisition.read();
    REQUIRE(plein_echelle.is_ok());
    CHECK_NEAR(static_cast<double>(plein_echelle.value()), 1023.75, 1e-3);
}

TEST_REQ(Acquisition, robustesse_hors_domaine, "LLR-CHAIN-010") {
    Acquisition acquisition;
    acquisition.set_raw(-1);
    CHECK_EQ(acquisition.read().status(), Status::OutOfRange);
    acquisition.set_raw(4096);
    CHECK_EQ(acquisition.read().status(), Status::OutOfRange);
    CHECK_EQ(acquisition.reject_count(), u32{2});
    CHECK_EQ(acquisition.read_count(), u32{2});
}

TEST_REQ(Filtre, fenetre_incomplete_ne_produit_rien, "LLR-CHAIN-021") {
    Filter filtre;
    for (usize index = 0U; index < (Filter::kWindow - 1U); ++index) {
        CHECK(filtre.push(100.0F));
        CHECK_EQ(filtre.average().status(), Status::NotReady);
    }
    CHECK(filtre.push(100.0F));
    CHECK(filtre.average().is_ok());
}

TEST_REQ(Filtre, moyenne_glissante, "LLR-CHAIN-020,LLR-CHAIN-021") {
    Filter filtre;
    CHECK(filtre.push(100.0F));
    CHECK(filtre.push(200.0F));
    CHECK(filtre.push(300.0F));
    CHECK(filtre.push(400.0F));

    const mod07::Result<f32> moyenne = filtre.average();
    REQUIRE(moyenne.is_ok());
    CHECK_NEAR(static_cast<double>(moyenne.value()), 250.0, 1e-4);

    // Cinquieme echantillon : le plus ancien sort de la fenetre.
    CHECK(filtre.push(500.0F));
    const mod07::Result<f32> glissee = filtre.average();
    REQUIRE(glissee.is_ok());
    CHECK_NEAR(static_cast<double>(glissee.value()), 350.0, 1e-4);
}

TEST_REQ(Filtre, robustesse_echantillon_non_fini, "LLR-CHAIN-020") {
    Filter filtre;
    CHECK_FALSE(filtre.push(kNaN));
    CHECK_EQ(filtre.sample_count(), usize{0});
}

TEST_REQ(Filtre, remise_a_zero, "LLR-CHAIN-022") {
    Filter filtre;
    for (usize index = 0U; index < Filter::kWindow; ++index) {
        (void)filtre.push(100.0F);
    }
    REQUIRE(filtre.average().is_ok());

    filtre.reset();
    CHECK_EQ(filtre.sample_count(), usize{0});
    CHECK_EQ(filtre.average().status(), Status::NotReady);
}

// =============================================================================
//  2. Tests d'INTEGRATION : le couplage
// =============================================================================
namespace {

/// Contexte d'integration instrumente. Les composants REELS sont utilises ;
/// seuls des decorateurs d'observation sont interposes.
struct ContexteTrace {
    Acquisition acquisition_reelle;
    Filter filtre_reel;
    mod12::TracingAcquisition acquisition{acquisition_reelle};
    mod12::TracingFilter filtre{filtre_reel};
    mod12::TracedSupervisor superviseur{acquisition, filtre};

    ContexteTrace() noexcept { CouplingTrace::reset(); }
};

}  // namespace

TEST_REQ(Couplage, cycle_nominal, "LLR-CHAIN-030") {
    ContexteTrace contexte;
    contexte.acquisition.set_raw(2000);

    const CycleOutcome resultat = contexte.superviseur.cycle();

    // Couplage de CONTROLE : trois interfaces exercees en un cycle.
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorReadsAcquisition), u32{1});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorPushesFilter), u32{1});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorReadsAverage), u32{1});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{0});

    // La fenetre n'est pas encore pleine.
    CHECK_EQ(resultat.status, Status::NotReady);
}

TEST_REQ(Couplage, donnee_transmise_sans_alteration, "LLR-CHAIN-030") {
    // COUPLAGE DE DONNEES : on verifie que la valeur produite par
    // Acquisition est EXACTEMENT celle recue par Filter. Une conversion
    // d'unite oubliee a cette frontiere serait invisible autrement.
    ContexteTrace contexte;
    contexte.acquisition.set_raw(2000);
    (void)contexte.superviseur.cycle();

    REQUIRE(CouplingTrace::data_count() >= 2U);

    Interface interface_lue = Interface::Count;
    f32 valeur_lue = 0.0F;
    REQUIRE(CouplingTrace::data_at(0U, interface_lue, valeur_lue));
    CHECK_EQ(interface_lue, Interface::SupervisorReadsAcquisition);
    CHECK_NEAR(static_cast<double>(valeur_lue), 500.0, 1e-3);

    Interface interface_poussee = Interface::Count;
    f32 valeur_poussee = 0.0F;
    REQUIRE(CouplingTrace::data_at(1U, interface_poussee, valeur_poussee));
    CHECK_EQ(interface_poussee, Interface::SupervisorPushesFilter);
    // MEME valeur, MEME unite : la frontiere ne transforme rien.
    CHECK_NEAR(static_cast<double>(valeur_poussee), static_cast<double>(valeur_lue), 1e-6);
}

TEST_REQ(Couplage, lecture_en_erreur, "LLR-CHAIN-030") {
    // Lecture en erreur : le filtre ne doit PAS etre sollicite. Ce test
    // verifie une ABSENCE d'appel, ce qu'aucun test unitaire ne peut faire.
    ContexteTrace contexte;
    contexte.acquisition.set_raw(9999);  // hors domaine

    const CycleOutcome resultat = contexte.superviseur.cycle();

    CHECK_EQ(resultat.status, Status::OutOfRange);
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorReadsAcquisition), u32{1});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorPushesFilter), u32{0});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorReadsAverage), u32{0});
}

TEST_REQ(Couplage, purge_apres_rejets, "LLR-CHAIN-031") {
    // LE cas d'integration : couplage de CONTROLE CONDITIONNEL, declenche par
    // une SEQUENCE. Aucun test unitaire de Filter ni d'Acquisition ne peut
    // l'exercer.
    ContexteTrace contexte;

    // Quatre mesures valides : la fenetre se remplit.
    contexte.acquisition.set_raw(2000);
    for (usize cycle = 0U; cycle < 4U; ++cycle) {
        (void)contexte.superviseur.cycle();
    }
    CHECK_EQ(contexte.filtre.sample_count(), Filter::kWindow);

    // Trois lectures en erreur consecutives -> purge.
    contexte.acquisition.set_raw(-5);
    (void)contexte.superviseur.cycle();
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{0});
    (void)contexte.superviseur.cycle();
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{0});
    (void)contexte.superviseur.cycle();

    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{1});
    CHECK_EQ(contexte.filtre.sample_count(), usize{0});
    CHECK_EQ(contexte.superviseur.flush_count(), u32{1});
}

TEST_REQ(Couplage, compteur_de_rejets_reinitialise, "LLR-CHAIN-031") {
    // Deux rejets, puis une lecture valide : le compteur doit repartir de zero.
    // Sans cette remise a zero, des rejets ISOLES finiraient par declencher la
    // purge -- exactement le defaut que le mot "consecutifs" doit empecher.
    ContexteTrace contexte;

    contexte.acquisition.set_raw(-5);
    (void)contexte.superviseur.cycle();
    (void)contexte.superviseur.cycle();
    CHECK_EQ(contexte.superviseur.consecutive_rejects(), u32{2});

    contexte.acquisition.set_raw(2000);
    (void)contexte.superviseur.cycle();
    CHECK_EQ(contexte.superviseur.consecutive_rejects(), u32{0});

    contexte.acquisition.set_raw(-5);
    (void)contexte.superviseur.cycle();
    (void)contexte.superviseur.cycle();
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{0});
}

TEST_REQ(Couplage, toutes_les_interfaces_exercees, "LLR-CHAIN-040") {
    // LA demonstration de l'objectif A-7.8 : une seule campagne, qui exerce
    // TOUTES les interfaces declarees dans le SDD.
    ContexteTrace contexte;

    contexte.acquisition.set_raw(2000);
    for (usize cycle = 0U; cycle < 5U; ++cycle) {
        (void)contexte.superviseur.cycle();
    }
    contexte.acquisition.set_raw(-1);
    for (usize cycle = 0U; cycle < 3U; ++cycle) {
        (void)contexte.superviseur.cycle();
    }

    CHECK_EQ(CouplingTrace::unexercised_count(), usize{0});
    CHECK(CouplingTrace::all_interfaces_exercised());

    for (avio::u8 index = 0U; index < CouplingTrace::kInterfaceCount; ++index) {
        const Interface interface = static_cast<Interface>(index);
        CHECK(CouplingTrace::call_count(interface) > 0U);
    }
}

TEST_REQ(Couplage, interface_non_exercee_detectee, "LLR-CHAIN-040") {
    // L'outil doit VRAIMENT detecter une interface manquante : sans ce test,
    // `all_interfaces_exercised()` pourrait renvoyer vrai en permanence et le
    // test precedent ne prouverait rien (DO-330 : verifier l'outil).
    ContexteTrace contexte;
    contexte.acquisition.set_raw(2000);
    (void)contexte.superviseur.cycle();  // n'exerce pas reset()

    CHECK_FALSE(CouplingTrace::all_interfaces_exercised());
    CHECK_EQ(CouplingTrace::unexercised_count(), usize{1});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{0});
}

// =============================================================================
//  3. Supervision : le comportement fonctionnel de l'assemblage
// =============================================================================
namespace {

/// Contexte d'integration NON instrumente : c'est la configuration EMBARQUEE.
struct ContexteEmbarque {
    Acquisition acquisition;
    Filter filtre;
    mod12::FlightSupervisor superviseur{acquisition, filtre};
};

}  // namespace

TEST_REQ(Supervision, alerte_au_dela_du_seuil, "LLR-CHAIN-032") {
    ContexteEmbarque contexte;
    // 3600 counts x 0,25 = 900 unites > 800
    contexte.acquisition.set_raw(3600);

    CycleOutcome resultat;
    for (usize cycle = 0U; cycle < Filter::kWindow; ++cycle) {
        resultat = contexte.superviseur.cycle();
    }
    REQUIRE_EQ(resultat.status, Status::Ok);
    CHECK_NEAR(static_cast<double>(resultat.filtered_value), 900.0, 1e-3);
    CHECK(resultat.alert);
}

TEST_REQ(Supervision, pas_d_alerte_sous_le_seuil, "LLR-CHAIN-032") {
    ContexteEmbarque contexte;
    contexte.acquisition.set_raw(2000);  // 500 unites

    CycleOutcome resultat;
    for (usize cycle = 0U; cycle < Filter::kWindow; ++cycle) {
        resultat = contexte.superviseur.cycle();
    }
    REQUIRE_EQ(resultat.status, Status::Ok);
    CHECK_FALSE(resultat.alert);
}

TEST_REQ(Supervision, seuil_exact_ne_declenche_pas, "LLR-CHAIN-032") {
    // 3200 counts x 0,25 = exactement 800,0. La condition est "> 800".
    ContexteEmbarque contexte;
    contexte.acquisition.set_raw(3200);

    CycleOutcome resultat;
    for (usize cycle = 0U; cycle < Filter::kWindow; ++cycle) {
        resultat = contexte.superviseur.cycle();
    }
    REQUIRE_EQ(resultat.status, Status::Ok);
    CHECK_NEAR(static_cast<double>(resultat.filtered_value), 800.0, 1e-3);
    CHECK_FALSE(resultat.alert);
}

TEST_REQ(Supervision, le_filtre_lisse_les_transitoires, "LLR-CHAIN-032") {
    // Comportement de l'ASSEMBLAGE : un pic isole ne doit pas declencher
    // l'alerte, parce que la moyenne l'attenue. Ni Acquisition ni Filter ne
    // possedent cette propriete a eux seuls.
    ContexteEmbarque contexte;

    const i32 profil[6] = {2000, 2000, 2000, 4000, 2000, 2000};
    CycleOutcome resultat;
    bool alerte_vue = false;
    for (usize cycle = 0U; cycle < 6U; ++cycle) {
        contexte.acquisition.set_raw(profil[cycle]);
        resultat = contexte.superviseur.cycle();
        if (resultat.alert) {
            alerte_vue = true;
        }
    }
    // Pic a 1000 unites, mais moyenne maximale = (500+500+500+1000)/4 = 625.
    CHECK_FALSE(alerte_vue);
}
