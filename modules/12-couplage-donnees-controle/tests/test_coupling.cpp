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
#include <avio/types.hpp>
#include <limits>
#include <microtest/microtest.hpp>

#include "mod12/chain.hpp"
#include "mod12/coupling_trace.hpp"

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
TEST_REQ(Acquisition, nominal_conversion, "LLR-CHAIN-010") {
    Acquisition acquisition;
    acquisition.set_raw(2000);
    const mod07::Result<f32> result = acquisition.read();
    REQUIRE(result.is_ok());
    CHECK_NEAR(static_cast<double>(result.value()), 500.0, 1e-3);
    CHECK_EQ(acquisition.read_count(), u32{1});
    CHECK_EQ(acquisition.reject_count(), u32{0});
}

TEST_REQ(Acquisition, converter_bounds, "LLR-CHAIN-010") {
    Acquisition acquisition;

    acquisition.set_raw(Acquisition::kRawMin);
    CHECK(acquisition.read().is_ok());

    acquisition.set_raw(Acquisition::kRawMax);
    const mod07::Result<f32> full_scale = acquisition.read();
    REQUIRE(full_scale.is_ok());
    CHECK_NEAR(static_cast<double>(full_scale.value()), 1023.75, 1e-3);
}

TEST_REQ(Acquisition, robustness_out_of_domain, "LLR-CHAIN-010") {
    Acquisition acquisition;
    acquisition.set_raw(-1);
    CHECK_EQ(acquisition.read().status(), Status::OutOfRange);
    acquisition.set_raw(4096);
    CHECK_EQ(acquisition.read().status(), Status::OutOfRange);
    CHECK_EQ(acquisition.reject_count(), u32{2});
    CHECK_EQ(acquisition.read_count(), u32{2});
}

TEST_REQ(Filter, incomplete_window_produces_nothing, "LLR-CHAIN-021") {
    Filter filter;
    for (usize index = 0U; index < (Filter::kWindow - 1U); ++index) {
        CHECK(filter.push(100.0F));
        CHECK_EQ(filter.average().status(), Status::NotReady);
    }
    CHECK(filter.push(100.0F));
    CHECK(filter.average().is_ok());
}

TEST_REQ(Filter, sliding_average, "LLR-CHAIN-020,LLR-CHAIN-021") {
    Filter filter;
    CHECK(filter.push(100.0F));
    CHECK(filter.push(200.0F));
    CHECK(filter.push(300.0F));
    CHECK(filter.push(400.0F));

    const mod07::Result<f32> average = filter.average();
    REQUIRE(average.is_ok());
    CHECK_NEAR(static_cast<double>(average.value()), 250.0, 1e-4);

    // Cinquieme echantillon : le plus ancien sort de la fenetre.
    CHECK(filter.push(500.0F));
    const mod07::Result<f32> shifted = filter.average();
    REQUIRE(shifted.is_ok());
    CHECK_NEAR(static_cast<double>(shifted.value()), 350.0, 1e-4);
}

TEST_REQ(Filter, robustness_non_finite_sample, "LLR-CHAIN-020") {
    Filter filter;
    CHECK_FALSE(filter.push(kNaN));
    CHECK_EQ(filter.sample_count(), usize{0});
}

TEST_REQ(Filter, reset_to_zero, "LLR-CHAIN-022") {
    Filter filter;
    for (usize index = 0U; index < Filter::kWindow; ++index) {
        (void)filter.push(100.0F);
    }
    REQUIRE(filter.average().is_ok());

    filter.reset();
    CHECK_EQ(filter.sample_count(), usize{0});
    CHECK_EQ(filter.average().status(), Status::NotReady);
}

// =============================================================================
//  2. Tests d'INTEGRATION : le couplage
// =============================================================================
namespace {

/// Contexte d'integration instrumente. Les composants REELS sont utilises ;
/// seuls des decorateurs d'observation sont interposes.
struct TraceContext {
    Acquisition real_acquisition;
    Filter real_filter;
    mod12::TracingAcquisition acquisition{real_acquisition};
    mod12::TracingFilter filter{real_filter};
    mod12::TracedSupervisor supervisor{acquisition, filter};

    TraceContext() noexcept { CouplingTrace::reset(); }
};

}  // namespace

TEST_REQ(Coupling, cycle_nominal, "LLR-CHAIN-030") {
    TraceContext context;
    context.acquisition.set_raw(2000);

    const CycleOutcome result = context.supervisor.cycle();

    // Couplage de CONTROLE : trois interfaces exercees en un cycle.
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorReadsAcquisition), u32{1});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorPushesFilter), u32{1});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorReadsAverage), u32{1});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{0});

    // La fenetre n'est pas encore pleine.
    CHECK_EQ(result.status, Status::NotReady);
}

TEST_REQ(Coupling, data_forwarded_without_alteration, "LLR-CHAIN-030") {
    // COUPLAGE DE DONNEES : on verifie que la valeur produite par
    // Acquisition est EXACTEMENT celle recue par Filter. Une conversion
    // d'unite oubliee a cette frontiere serait invisible autrement.
    TraceContext context;
    context.acquisition.set_raw(2000);
    (void)context.supervisor.cycle();

    REQUIRE(CouplingTrace::data_count() >= 2U);

    Interface interface_read = Interface::Count;
    f32 value_read = 0.0F;
    REQUIRE(CouplingTrace::data_at(0U, interface_read, value_read));
    CHECK_EQ(interface_read, Interface::SupervisorReadsAcquisition);
    CHECK_NEAR(static_cast<double>(value_read), 500.0, 1e-3);

    Interface pushed_interface = Interface::Count;
    f32 pushed_value = 0.0F;
    REQUIRE(CouplingTrace::data_at(1U, pushed_interface, pushed_value));
    CHECK_EQ(pushed_interface, Interface::SupervisorPushesFilter);
    // MEME valeur, MEME unite : la frontiere ne transforme rien.
    CHECK_NEAR(static_cast<double>(pushed_value), static_cast<double>(value_read), 1e-6);
}

TEST_REQ(Coupling, read_in_error, "LLR-CHAIN-030") {
    // Lecture en erreur : le filtre ne doit PAS etre sollicite. Ce test
    // verifie une ABSENCE d'appel, ce qu'aucun test unitaire ne peut faire.
    TraceContext context;
    context.acquisition.set_raw(9999);  // hors domaine

    const CycleOutcome result = context.supervisor.cycle();

    CHECK_EQ(result.status, Status::OutOfRange);
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorReadsAcquisition), u32{1});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorPushesFilter), u32{0});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorReadsAverage), u32{0});
}

TEST_REQ(Coupling, purge_after_rejections, "LLR-CHAIN-031") {
    // LE cas d'integration : couplage de CONTROLE CONDITIONNEL, declenche par
    // une SEQUENCE. Aucun test unitaire de Filter ni d'Acquisition ne peut
    // l'exercer.
    TraceContext context;

    // Quatre mesures valides : la fenetre se remplit.
    context.acquisition.set_raw(2000);
    for (usize cycle = 0U; cycle < 4U; ++cycle) {
        (void)context.supervisor.cycle();
    }
    CHECK_EQ(context.filter.sample_count(), Filter::kWindow);

    // Trois lectures en erreur consecutives -> purge.
    context.acquisition.set_raw(-5);
    (void)context.supervisor.cycle();
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{0});
    (void)context.supervisor.cycle();
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{0});
    (void)context.supervisor.cycle();

    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{1});
    CHECK_EQ(context.filter.sample_count(), usize{0});
    CHECK_EQ(context.supervisor.flush_count(), u32{1});
}

TEST_REQ(Coupling, rejection_counter_reset, "LLR-CHAIN-031") {
    // Deux rejets, puis une lecture valide : le compteur doit repartir de zero.
    // Sans cette remise a zero, des rejets ISOLES finiraient par declencher la
    // purge -- exactement le defaut que le mot "consecutifs" doit empecher.
    TraceContext context;

    context.acquisition.set_raw(-5);
    (void)context.supervisor.cycle();
    (void)context.supervisor.cycle();
    CHECK_EQ(context.supervisor.consecutive_rejects(), u32{2});

    context.acquisition.set_raw(2000);
    (void)context.supervisor.cycle();
    CHECK_EQ(context.supervisor.consecutive_rejects(), u32{0});

    context.acquisition.set_raw(-5);
    (void)context.supervisor.cycle();
    (void)context.supervisor.cycle();
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{0});
}

TEST_REQ(Coupling, all_interfaces_exercised, "LLR-CHAIN-040") {
    // LA demonstration de l'objectif A-7.8 : une seule campagne, qui exerce
    // TOUTES les interfaces declarees dans le SDD.
    TraceContext context;

    context.acquisition.set_raw(2000);
    for (usize cycle = 0U; cycle < 5U; ++cycle) {
        (void)context.supervisor.cycle();
    }
    context.acquisition.set_raw(-1);
    for (usize cycle = 0U; cycle < 3U; ++cycle) {
        (void)context.supervisor.cycle();
    }

    CHECK_EQ(CouplingTrace::unexercised_count(), usize{0});
    CHECK(CouplingTrace::all_interfaces_exercised());

    for (avio::u8 index = 0U; index < CouplingTrace::kInterfaceCount; ++index) {
        const Interface interface = static_cast<Interface>(index);
        CHECK(CouplingTrace::call_count(interface) > 0U);
    }
}

TEST_REQ(Coupling, interface_not_exercised_detected, "LLR-CHAIN-040") {
    // L'outil doit VRAIMENT detecter une interface manquante : sans ce test,
    // `all_interfaces_exercised()` pourrait renvoyer vrai en permanence et le
    // test precedent ne prouverait rien (DO-330 : verifier l'outil).
    TraceContext context;
    context.acquisition.set_raw(2000);
    (void)context.supervisor.cycle();  // n'exerce pas reset()

    CHECK_FALSE(CouplingTrace::all_interfaces_exercised());
    CHECK_EQ(CouplingTrace::unexercised_count(), usize{1});
    CHECK_EQ(CouplingTrace::call_count(Interface::SupervisorResetsFilter), u32{0});
}

// =============================================================================
//  3. Supervision : le comportement fonctionnel de l'assemblage
// =============================================================================
namespace {

/// Contexte d'integration NON instrumente : c'est la configuration EMBARQUEE.
struct EmbeddedContext {
    Acquisition acquisition;
    Filter filter;
    mod12::FlightSupervisor supervisor{acquisition, filter};
};

}  // namespace

TEST_REQ(Supervision, alert_beyond_threshold, "LLR-CHAIN-032") {
    EmbeddedContext context;
    // 3600 counts x 0,25 = 900 unites > 800
    context.acquisition.set_raw(3600);

    CycleOutcome result;
    for (usize cycle = 0U; cycle < Filter::kWindow; ++cycle) {
        result = context.supervisor.cycle();
    }
    REQUIRE_EQ(result.status, Status::Ok);
    CHECK_NEAR(static_cast<double>(result.filtered_value), 900.0, 1e-3);
    CHECK(result.alert);
}

TEST_REQ(Supervision, no_alert_below_threshold, "LLR-CHAIN-032") {
    EmbeddedContext context;
    context.acquisition.set_raw(2000);  // 500 unites

    CycleOutcome result;
    for (usize cycle = 0U; cycle < Filter::kWindow; ++cycle) {
        result = context.supervisor.cycle();
    }
    REQUIRE_EQ(result.status, Status::Ok);
    CHECK_FALSE(result.alert);
}

TEST_REQ(Supervision, exact_threshold_does_not_trigger, "LLR-CHAIN-032") {
    // 3200 counts x 0,25 = exactement 800,0. La condition est "> 800".
    EmbeddedContext context;
    context.acquisition.set_raw(3200);

    CycleOutcome result;
    for (usize cycle = 0U; cycle < Filter::kWindow; ++cycle) {
        result = context.supervisor.cycle();
    }
    REQUIRE_EQ(result.status, Status::Ok);
    CHECK_NEAR(static_cast<double>(result.filtered_value), 800.0, 1e-3);
    CHECK_FALSE(result.alert);
}

TEST_REQ(Supervision, the_filter_smooths_transients, "LLR-CHAIN-032") {
    // Comportement de l'ASSEMBLAGE : un pic isole ne doit pas declencher
    // l'alerte, parce que la moyenne l'attenue. Ni Acquisition ni Filter ne
    // possedent cette propriete a eux seuls.
    EmbeddedContext context;

    const i32 profile[6] = {2000, 2000, 2000, 4000, 2000, 2000};
    CycleOutcome result;
    bool alert_seen = false;
    for (usize cycle = 0U; cycle < 6U; ++cycle) {
        context.acquisition.set_raw(profile[cycle]);
        result = context.supervisor.cycle();
        if (result.alert) {
            alert_seen = true;
        }
    }
    // Pic a 1000 unites, mais moyenne maximale = (500+500+500+1000)/4 = 625.
    CHECK_FALSE(alert_seen);
}
