#include <avio/types.hpp>
#include <microtest/microtest.hpp>

#include "mod05/sensors.hpp"

using avio::f32;
using avio::i32;
using avio::u16;

// =============================================================================
//  1. Tests "classiques" du type de base, ecrits UNE FOIS
// =============================================================================
namespace {

/// Campagne de test du contrat de `Sensor`, applicable a tout sous-type.
/// C'est l'implementation du TEST PESSIMISTE de la DO-332 : au lieu d'ecrire
/// une campagne par sous-type, on ecrit la campagne du TYPE DE BASE et on la
/// rejoue sur chaque sous-type.
void check_conforming_subtype(const mod05::Sensor& sensor) {
    const mod05::ContractReport report = mod05::verify_contract(sensor, 200U);

    CHECK(report.c1_name_valid);
    CHECK(report.c2_raw_domain_valid);
    CHECK(report.c3_value_domain_valid);
    CHECK(report.c4_result_in_domain);
    CHECK(report.c5_monotonic);
    CHECK(report.c6_endpoints_match);
    CHECK(report.c7_clamped_outside);

    if (!report.all_satisfied()) {
        FAIL("coherence locale de type violee");
    }
}

}  // namespace

// -----------------------------------------------------------------------------
//  Substituabilite : chaque sous-type rejoue la campagne de la base
// -----------------------------------------------------------------------------
TEST_REQ(DO332, pressure_sensor_is_substitutable, "LLR-M05-001,OO.6.7") {
    const mod05::PressureSensor sensor;
    check_conforming_subtype(sensor);
}

TEST_REQ(DO332, temperature_sensor_is_substitutable, "LLR-M05-002,OO.6.7") {
    const mod05::TemperatureSensor sensor;
    check_conforming_subtype(sensor);
}

TEST_REQ(DO332, the_harness_detects_a_violation, "LLR-M05-003,OO.6.7") {
    // Test de l'OUTIL, pas du capteur : si le harnais ne detectait pas ce
    // capteur fautif, les deux tests precedents ne prouveraient rien.
    // C'est le principe du "test du test" (DO-330 applique au harnais).
    const mod05::BrokenSensor sensor;
    const mod05::ContractReport report = mod05::verify_contract(sensor, 200U);

    CHECK_FALSE(report.all_satisfied());
    CHECK_FALSE(report.c5_monotonic);        // la courbe redescend
    CHECK_FALSE(report.c6_endpoints_match);  // to_engineering(raw_max) != value_max
    CHECK_FALSE(report.c7_clamped_outside);  // pas d'ecretage hors domaine
    CHECK_EQ(report.first_violation(), "C5");
}

TEST_REQ(DO332, robustness_degenerate_sampling, "LLR-M05-004") {
    // sample_count < 2 : le harnais doit rester utilisable et deterministe.
    const mod05::PressureSensor sensor;
    const mod05::ContractReport report = mod05::verify_contract(sensor, 0U);
    CHECK(report.c2_raw_domain_valid);
    CHECK(report.c6_endpoints_match);
}

// -----------------------------------------------------------------------------
//  Comportement propre a chaque sous-type
// -----------------------------------------------------------------------------
TEST_REQ(Pressure, linear_conversion, "LLR-M05-010") {
    const mod05::PressureSensor sensor;
    CHECK_NEAR(static_cast<double>(sensor.to_engineering(0)), 0.0, 1e-3);
    CHECK_NEAR(static_cast<double>(sensor.to_engineering(4095)), 1200.0, 1e-3);
    CHECK_NEAR(static_cast<double>(sensor.to_engineering(2047)), 599.85, 0.5);
}

TEST_REQ(Pressure, clamping_out_of_domain, "LLR-M05-011") {
    const mod05::PressureSensor sensor;
    CHECK_NEAR(static_cast<double>(sensor.to_engineering(-1)), 0.0, 1e-3);
    CHECK_NEAR(static_cast<double>(sensor.to_engineering(-100000)), 0.0, 1e-3);
    CHECK_NEAR(static_cast<double>(sensor.to_engineering(100000)), 1200.0, 1e-3);
}

TEST_REQ(Temperature, conversion_with_shift, "LLR-M05-012") {
    const mod05::TemperatureSensor sensor;
    CHECK_NEAR(static_cast<double>(sensor.to_engineering(0)), -60.0, 1e-3);
    CHECK_NEAR(static_cast<double>(sensor.to_engineering(4095)), 80.0, 1e-3);
    // Milieu du domaine : -60 + 0,5 * 140 = 10
    CHECK_NEAR(static_cast<double>(sensor.to_engineering(2047)), 10.0, 0.1);
}

TEST_REQ(Sensor, common_non_virtual_method, "LLR-M05-013") {
    const mod05::PressureSensor sensor;
    CHECK(sensor.is_in_range(0));
    CHECK(sensor.is_in_range(4095));
    CHECK_FALSE(sensor.is_in_range(-1));
    CHECK_FALSE(sensor.is_in_range(4096));
}

// -----------------------------------------------------------------------------
//  Resolution dynamique a travers une reference de base
// -----------------------------------------------------------------------------
TEST_REQ(Polymorphism, dynamic_resolution, "LLR-M05-020") {
    const mod05::PressureSensor pressure;
    const mod05::TemperatureSensor temperature;

    // Un tableau de POINTEURS vers la base : la ou le polymorphisme sert
    // vraiment. Aucune allocation dynamique : les objets vivent sur la pile.
    const mod05::Sensor* sensors[2] = {&pressure, &temperature};

    CHECK_EQ(sensors[0]->name(), "Pression");
    CHECK_EQ(sensors[1]->name(), "Temperature");
    CHECK_NEAR(static_cast<double>(sensors[0]->to_engineering(0)), 0.0, 1e-3);
    CHECK_NEAR(static_cast<double>(sensors[1]->to_engineering(0)), -60.0, 1e-3);
}

TEST_REQ(Polymorphism, vtable_pointer_memory_cost, "LLR-M05-021") {
    // Une classe polymorphe contient un pointeur cache vers sa table de
    // fonctions virtuelles. C'est 8 octets par OBJET sur une cible 64 bits,
    // 4 octets sur une cible 32 bits. Sur 10 000 objets, cela compte.
    CHECK(sizeof(mod05::PressureSensor) >= sizeof(void*));
    CHECK_EQ(sizeof(mod05::PressureSensor), sizeof(void*));
}

// -----------------------------------------------------------------------------
//  Decoupage (slicing)
// -----------------------------------------------------------------------------
TEST_REQ(Slicing, pass_by_value_loses_the_subtype, "LLR-M05-030") {
    const mod05::ExtendedMessage message(0x101U, 20U);

    CHECK_EQ(message.length(), u16{24U});

    // DEVIATION JUSTIFIEE (cppcoreguidelines-slicing) : le decoupage est
    // precisement le DEFAUT que ce test doit mettre en evidence. Sans cet
    // appel, rien ne demontrerait le probleme.
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    const u16 by_value = mod05::length_by_value(message);
    const u16 by_reference = mod05::length_by_reference(message);

    CHECK_EQ(by_value, u16{4U});       // DECOUPE : seule la base a survecu
    CHECK_EQ(by_reference, u16{24U});  // correct
    CHECK(by_value != by_reference);
}

TEST_REQ(Slicing, the_base_stays_correct, "LLR-M05-031") {
    const mod05::Message base(0x200U);
    CHECK_EQ(mod05::length_by_value(base), u16{4U});
    CHECK_EQ(mod05::length_by_reference(base), u16{4U});
    CHECK_EQ(base.identifier(), u16{0x200U});
}
