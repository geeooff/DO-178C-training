#include <avio/types.hpp>
#include <limits>
#include <microtest/microtest.hpp>

#include "mod04/units.hpp"

using avio::f32;
using avio::i32;
using mod04::Altitude;
using mod04::FuelTank;
using mod04::Mass;

namespace {

constexpr f32 kNaN = std::numeric_limits<f32>::quiet_NaN();
constexpr f32 kInf = std::numeric_limits<f32>::infinity();

Mass grams(i32 value) noexcept {
    Mass mass;
    (void)Mass::from_grams(value, mass);
    return mass;
}

}  // namespace

// -----------------------------------------------------------------------------
//  Mass : fabriques et domaine
// -----------------------------------------------------------------------------
TEST_REQ(Mass, valid_default_state, "LLR-M04-001") {
    const Mass mass;
    CHECK_EQ(mass.grams(), 0);
    // Un objet par defaut doit deja respecter son invariant : pas d'etat
    // "non initialise" observable de l'exterieur.
}

TEST_REQ(Mass, from_grams_nominal, "LLR-M04-002") {
    Mass mass;
    REQUIRE(Mass::from_grams(1500, mass));
    CHECK_EQ(mass.grams(), 1500);
    CHECK_NEAR(static_cast<double>(mass.kilograms()), 1.5, 1e-6);
}

TEST_REQ(Mass, from_kilograms_nominal, "LLR-M04-003") {
    Mass mass;
    REQUIRE(Mass::from_kilograms(2.5F, mass));
    CHECK_EQ(mass.grams(), 2500);
}

TEST_REQ(Mass, from_pounds_nominal, "LLR-M04-004") {
    Mass mass;
    REQUIRE(Mass::from_pounds(10.0F, mass));
    // 10 lb = 4535,9237 g -> tronque a 4535 g
    CHECK_EQ(mass.grams(), 4535);
    CHECK_NEAR(static_cast<double>(mass.pounds()), 10.0, 1e-2);
}

TEST_REQ(Mass, robustness_negative_value, "LLR-M04-005") {
    Mass mass;
    CHECK_FALSE(Mass::from_grams(-1, mass));
    CHECK_FALSE(Mass::from_kilograms(-0.001F, mass));
    CHECK_FALSE(Mass::from_pounds(-100.0F, mass));
    CHECK_EQ(mass.grams(), 0);  // la sortie n'a pas ete polluee
}

TEST_REQ(Mass, robustness_out_of_domain_high, "LLR-M04-006") {
    Mass mass;
    CHECK(Mass::from_grams(Mass::kMaxGrams, mass));
    CHECK_FALSE(Mass::from_grams(Mass::kMaxGrams + 1, mass));
    CHECK_FALSE(Mass::from_kilograms(1.0e9F, mass));
}

TEST_REQ(Mass, robustness_nan_and_infinity, "LLR-M04-007") {
    // NaN et l'infini traversent silencieusement toute arithmetique flottante.
    // Les arreter A L'ENTREE est la seule strategie tenable : c'est le principe
    // de la "validation aux frontieres" (module 15).
    Mass mass;
    CHECK_FALSE(Mass::from_kilograms(kNaN, mass));
    CHECK_FALSE(Mass::from_kilograms(kInf, mass));
    CHECK_FALSE(Mass::from_kilograms(-kInf, mass));
    CHECK_FALSE(Mass::from_pounds(kNaN, mass));
}

// -----------------------------------------------------------------------------
//  Mass : operateurs
// -----------------------------------------------------------------------------
TEST_REQ(Mass, comparisons, "LLR-M04-010") {
    const Mass small = grams(100);
    const Mass large = grams(200);
    const Mass identical = grams(100);

    CHECK(small == identical);
    CHECK(small != large);
    CHECK(small < large);
    CHECK(large > small);
    CHECK(small <= identical);
    CHECK(large >= small);
}

TEST_REQ(Mass, addition_saturating, "LLR-M04-011") {
    CHECK_EQ((grams(100) + grams(50)).grams(), 150);
    const Mass maximum = grams(Mass::kMaxGrams);
    CHECK_EQ((maximum + grams(1000)).grams(), Mass::kMaxGrams);
}

TEST_REQ(Mass, subtraction_clamped_at_zero, "LLR-M04-012") {
    CHECK_EQ((grams(200) - grams(50)).grams(), 150);
    // Une masse ne peut pas devenir negative : c'est un invariant du TYPE,
    // pas une convention laissee a l'appelant.
    CHECK_EQ((grams(50) - grams(200)).grams(), 0);
}

// -----------------------------------------------------------------------------
//  Altitude
// -----------------------------------------------------------------------------
TEST_REQ(Altitude, from_feet_nominal, "LLR-M04-020") {
    Altitude altitude;
    REQUIRE(Altitude::from_feet(35000.0F, altitude));
    CHECK_NEAR(static_cast<double>(altitude.feet()), 35000.0, 1e-3);
    CHECK_NEAR(static_cast<double>(altitude.meters()), 10668.0, 1.0);
}

TEST_REQ(Altitude, from_meters_nominal, "LLR-M04-021") {
    Altitude altitude;
    REQUIRE(Altitude::from_meters(1000.0F, altitude));
    CHECK_NEAR(static_cast<double>(altitude.feet()), 3280.84, 0.1);
}

TEST_REQ(Altitude, domain_bounds, "LLR-M04-022") {
    Altitude altitude;
    CHECK(Altitude::from_feet(Altitude::kMinFeet, altitude));
    CHECK(Altitude::from_feet(Altitude::kMaxFeet, altitude));
    CHECK_FALSE(Altitude::from_feet(Altitude::kMinFeet - 1.0F, altitude));
    CHECK_FALSE(Altitude::from_feet(Altitude::kMaxFeet + 1.0F, altitude));
}

TEST_REQ(Altitude, robustness_nan_and_infinity, "LLR-M04-023") {
    Altitude altitude;
    CHECK_FALSE(Altitude::from_feet(kNaN, altitude));
    CHECK_FALSE(Altitude::from_feet(kInf, altitude));
    CHECK_FALSE(Altitude::from_meters(kNaN, altitude));
    CHECK_FALSE(Altitude::from_meters(kInf, altitude));
}

TEST_REQ(Altitude, comparison_with_tolerance, "LLR-M04-024") {
    Altitude a;
    Altitude b;
    REQUIRE(Altitude::from_feet(10000.0F, a));
    REQUIRE(Altitude::from_feet(10000.4F, b));

    CHECK(a.is_close(b, 1.0F));
    CHECK_FALSE(a.is_close(b, 0.1F));
    // Tolerance nulle : seule l'egalite bit a bit passe. C'est justement ce
    // que l'on veut EVITER d'ecrire par defaut.
    CHECK(a.is_close(a, 0.0F));
}

TEST_REQ(Altitude, robustness_invalid_tolerance, "LLR-M04-025") {
    Altitude a;
    REQUIRE(Altitude::from_feet(10000.0F, a));
    CHECK_FALSE(a.is_close(a, -1.0F));
    CHECK_FALSE(a.is_close(a, kNaN));
}

TEST_REQ(Altitude, total_order, "LLR-M04-026") {
    Altitude low;
    Altitude high;
    REQUIRE(Altitude::from_feet(1000.0F, low));
    REQUIRE(Altitude::from_feet(30000.0F, high));

    CHECK(low < high);
    CHECK(high > low);
    CHECK(low <= high);
    CHECK(high >= low);
    CHECK_NEAR(static_cast<double>(high.difference_feet(low)), 29000.0, 1e-2);
    CHECK_NEAR(static_cast<double>(low.difference_feet(high)), -29000.0, 1e-2);
}

TEST_REQ(Altitude, round_trip_feet_meters, "LLR-M04-027") {
    Altitude start;
    REQUIRE(Altitude::from_feet(25000.0F, start));

    Altitude back;
    REQUIRE(Altitude::from_meters(start.meters(), back));

    // La conversion aller-retour introduit une erreur d'arrondi : elle est
    // BORNEE et on la specifie. Exiger l'egalite exacte serait une faute.
    CHECK(start.is_close(back, 0.1F));
}

// -----------------------------------------------------------------------------
//  FuelTank : maintien de l'invariant
// -----------------------------------------------------------------------------
TEST_REQ(FuelTank, nominal_creation, "LLR-M04-030") {
    FuelTank tank;
    REQUIRE(FuelTank::create(grams(10000), tank));
    CHECK_EQ(tank.capacity().grams(), 10000);
    CHECK_EQ(tank.quantity().grams(), 0);
    CHECK(tank.is_empty());
    CHECK_FALSE(tank.is_full());
    CHECK(tank.invariant_holds());
}

TEST_REQ(FuelTank, robustness_zero_capacity, "LLR-M04-031") {
    FuelTank tank;
    CHECK_FALSE(FuelTank::create(Mass(), tank));
    // Le reservoir degenere reste neanmoins dans un etat coherent.
    CHECK(tank.invariant_holds());
    CHECK_NEAR(static_cast<double>(tank.fill_ratio_percent()), 0.0, 1e-6);
}

TEST_REQ(FuelTank, partial_filling, "LLR-M04-032") {
    FuelTank tank;
    REQUIRE(FuelTank::create(grams(10000), tank));

    const Mass added = tank.add(grams(2500));
    CHECK_EQ(added.grams(), 2500);
    CHECK_EQ(tank.quantity().grams(), 2500);
    CHECK_NEAR(static_cast<double>(tank.fill_ratio_percent()), 25.0, 1e-4);
    CHECK(tank.invariant_holds());
}

TEST_REQ(FuelTank, overflow_on_add, "LLR-M04-033") {
    FuelTank tank;
    REQUIRE(FuelTank::create(grams(1000), tank));

    const Mass added = tank.add(grams(5000));
    // Seule la place disponible est acceptee : l'invariant tient.
    CHECK_EQ(added.grams(), 1000);
    CHECK_EQ(tank.quantity().grams(), 1000);
    CHECK(tank.is_full());
    CHECK(tank.invariant_holds());
}

TEST_REQ(FuelTank, withdrawal_greater_than_content, "LLR-M04-034") {
    FuelTank tank;
    REQUIRE(FuelTank::create(grams(1000), tank));
    (void)tank.add(grams(300));

    const Mass removed = tank.remove(grams(900));
    CHECK_EQ(removed.grams(), 300);
    CHECK(tank.is_empty());
    CHECK(tank.invariant_holds());
}

TEST_REQ(FuelTank, long_sequence_invariant_always_true, "LLR-M04-035") {
    // Test de propriete : quelle que soit la sequence d'operations, y compris
    // absurde, l'invariant doit tenir. C'est ce type de test qui donne
    // confiance dans une classe a etat.
    FuelTank tank;
    REQUIRE(FuelTank::create(grams(5000), tank));

    const i32 operations[10] = {1000, -3000, 4000, 4000, -100, -100, 9000, -9000, 250, -250};
    for (i32 operation : operations) {
        if (operation >= 0) {
            (void)tank.add(grams(operation));
        } else {
            (void)tank.remove(grams(-operation));
        }
        REQUIRE(tank.invariant_holds());
        REQUIRE(tank.quantity() <= tank.capacity());
    }
}

// -----------------------------------------------------------------------------
//  Le type fort empeche la confusion d'unites
// -----------------------------------------------------------------------------
TEST_REQ(StrongType, same_mass_two_units, "LLR-M04-040") {
    // Le scenario du "Gimli Glider" : 22 300 unites de carburant.
    // Interpretees en livres au lieu de kilogrammes, cela fait moins de la
    // moitie de la masse attendue.
    Mass pounds;
    Mass kilograms;
    REQUIRE(Mass::from_pounds(22300.0F, pounds));
    REQUIRE(Mass::from_kilograms(22300.0F, kilograms));

    CHECK(pounds < kilograms);
    CHECK_NEAR(static_cast<double>(pounds.kilograms()), 10115.0, 1.0);
    CHECK_NEAR(static_cast<double>(kilograms.kilograms()), 22300.0, 1.0);

    // Une fois construites, les deux masses sont comparables SANS ambiguite :
    // l'unite a ete fixee a la frontiere du systeme, une seule fois.
    const Mass missing = kilograms - pounds;
    CHECK_NEAR(static_cast<double>(missing.kilograms()), 12184.9, 1.0);
}
