#include <microtest/microtest.hpp>

#include "mod04/units.hpp"

#include <avio/types.hpp>

#include <limits>

using avio::f32;
using avio::i32;
using mod04::Altitude;
using mod04::FuelTank;
using mod04::Mass;

namespace {

constexpr f32 kNaN = std::numeric_limits<f32>::quiet_NaN();
constexpr f32 kInf = std::numeric_limits<f32>::infinity();

Mass grams(i32 value) noexcept {
    Mass masse;
    (void)Mass::from_grams(value, masse);
    return masse;
}

}  // namespace

// -----------------------------------------------------------------------------
//  Mass : fabriques et domaine
// -----------------------------------------------------------------------------
TEST_REQ(Mass, etat_par_defaut_valide, "LLR-M04-001") {
    const Mass masse;
    CHECK_EQ(masse.grams(), 0);
    // Un objet par defaut doit deja respecter son invariant : pas d'etat
    // "non initialise" observable de l'exterieur.
}

TEST_REQ(Mass, fabrique_grammes_nominale, "LLR-M04-002") {
    Mass masse;
    REQUIRE(Mass::from_grams(1500, masse));
    CHECK_EQ(masse.grams(), 1500);
    CHECK_NEAR(static_cast<double>(masse.kilograms()), 1.5, 1e-6);
}

TEST_REQ(Mass, fabrique_kilogrammes_nominale, "LLR-M04-003") {
    Mass masse;
    REQUIRE(Mass::from_kilograms(2.5F, masse));
    CHECK_EQ(masse.grams(), 2500);
}

TEST_REQ(Mass, fabrique_livres_nominale, "LLR-M04-004") {
    Mass masse;
    REQUIRE(Mass::from_pounds(10.0F, masse));
    // 10 lb = 4535,9237 g -> tronque a 4535 g
    CHECK_EQ(masse.grams(), 4535);
    CHECK_NEAR(static_cast<double>(masse.pounds()), 10.0, 1e-2);
}

TEST_REQ(Mass, robustesse_valeur_negative, "LLR-M04-005") {
    Mass masse;
    CHECK_FALSE(Mass::from_grams(-1, masse));
    CHECK_FALSE(Mass::from_kilograms(-0.001F, masse));
    CHECK_FALSE(Mass::from_pounds(-100.0F, masse));
    CHECK_EQ(masse.grams(), 0);  // la sortie n'a pas ete polluee
}

TEST_REQ(Mass, robustesse_hors_domaine_haut, "LLR-M04-006") {
    Mass masse;
    CHECK(Mass::from_grams(Mass::kMaxGrams, masse));
    CHECK_FALSE(Mass::from_grams(Mass::kMaxGrams + 1, masse));
    CHECK_FALSE(Mass::from_kilograms(1.0e9F, masse));
}

TEST_REQ(Mass, robustesse_nan_et_infini, "LLR-M04-007") {
    // NaN et l'infini traversent silencieusement toute arithmetique flottante.
    // Les arreter A L'ENTREE est la seule strategie tenable : c'est le principe
    // de la "validation aux frontieres" (module 15).
    Mass masse;
    CHECK_FALSE(Mass::from_kilograms(kNaN, masse));
    CHECK_FALSE(Mass::from_kilograms(kInf, masse));
    CHECK_FALSE(Mass::from_kilograms(-kInf, masse));
    CHECK_FALSE(Mass::from_pounds(kNaN, masse));
}

// -----------------------------------------------------------------------------
//  Mass : operateurs
// -----------------------------------------------------------------------------
TEST_REQ(Mass, comparaisons, "LLR-M04-010") {
    const Mass petite = grams(100);
    const Mass grande = grams(200);
    const Mass identique = grams(100);

    CHECK(petite == identique);
    CHECK(petite != grande);
    CHECK(petite < grande);
    CHECK(grande > petite);
    CHECK(petite <= identique);
    CHECK(grande >= petite);
}

TEST_REQ(Mass, addition_saturante, "LLR-M04-011") {
    CHECK_EQ((grams(100) + grams(50)).grams(), 150);
    const Mass maximum = grams(Mass::kMaxGrams);
    CHECK_EQ((maximum + grams(1000)).grams(), Mass::kMaxGrams);
}

TEST_REQ(Mass, soustraction_bornee_a_zero, "LLR-M04-012") {
    CHECK_EQ((grams(200) - grams(50)).grams(), 150);
    // Une masse ne peut pas devenir negative : c'est un invariant du TYPE,
    // pas une convention laissee a l'appelant.
    CHECK_EQ((grams(50) - grams(200)).grams(), 0);
}

// -----------------------------------------------------------------------------
//  Altitude
// -----------------------------------------------------------------------------
TEST_REQ(Altitude, fabrique_pieds_nominale, "LLR-M04-020") {
    Altitude altitude;
    REQUIRE(Altitude::from_feet(35000.0F, altitude));
    CHECK_NEAR(static_cast<double>(altitude.feet()), 35000.0, 1e-3);
    CHECK_NEAR(static_cast<double>(altitude.meters()), 10668.0, 1.0);
}

TEST_REQ(Altitude, fabrique_metres_nominale, "LLR-M04-021") {
    Altitude altitude;
    REQUIRE(Altitude::from_meters(1000.0F, altitude));
    CHECK_NEAR(static_cast<double>(altitude.feet()), 3280.84, 0.1);
}

TEST_REQ(Altitude, bornes_du_domaine, "LLR-M04-022") {
    Altitude altitude;
    CHECK(Altitude::from_feet(Altitude::kMinFeet, altitude));
    CHECK(Altitude::from_feet(Altitude::kMaxFeet, altitude));
    CHECK_FALSE(Altitude::from_feet(Altitude::kMinFeet - 1.0F, altitude));
    CHECK_FALSE(Altitude::from_feet(Altitude::kMaxFeet + 1.0F, altitude));
}

TEST_REQ(Altitude, robustesse_nan_et_infini, "LLR-M04-023") {
    Altitude altitude;
    CHECK_FALSE(Altitude::from_feet(kNaN, altitude));
    CHECK_FALSE(Altitude::from_feet(kInf, altitude));
    CHECK_FALSE(Altitude::from_meters(kNaN, altitude));
    CHECK_FALSE(Altitude::from_meters(kInf, altitude));
}

TEST_REQ(Altitude, comparaison_a_tolerance, "LLR-M04-024") {
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

TEST_REQ(Altitude, robustesse_tolerance_invalide, "LLR-M04-025") {
    Altitude a;
    REQUIRE(Altitude::from_feet(10000.0F, a));
    CHECK_FALSE(a.is_close(a, -1.0F));
    CHECK_FALSE(a.is_close(a, kNaN));
}

TEST_REQ(Altitude, ordre_total, "LLR-M04-026") {
    Altitude basse;
    Altitude haute;
    REQUIRE(Altitude::from_feet(1000.0F, basse));
    REQUIRE(Altitude::from_feet(30000.0F, haute));

    CHECK(basse < haute);
    CHECK(haute > basse);
    CHECK(basse <= haute);
    CHECK(haute >= basse);
    CHECK_NEAR(static_cast<double>(haute.difference_feet(basse)), 29000.0, 1e-2);
    CHECK_NEAR(static_cast<double>(basse.difference_feet(haute)), -29000.0, 1e-2);
}

TEST_REQ(Altitude, aller_retour_pieds_metres, "LLR-M04-027") {
    Altitude depart;
    REQUIRE(Altitude::from_feet(25000.0F, depart));

    Altitude retour;
    REQUIRE(Altitude::from_meters(depart.meters(), retour));

    // La conversion aller-retour introduit une erreur d'arrondi : elle est
    // BORNEE et on la specifie. Exiger l'egalite exacte serait une faute.
    CHECK(depart.is_close(retour, 0.1F));
}

// -----------------------------------------------------------------------------
//  FuelTank : maintien de l'invariant
// -----------------------------------------------------------------------------
TEST_REQ(FuelTank, creation_nominale, "LLR-M04-030") {
    FuelTank reservoir;
    REQUIRE(FuelTank::create(grams(10000), reservoir));
    CHECK_EQ(reservoir.capacity().grams(), 10000);
    CHECK_EQ(reservoir.quantity().grams(), 0);
    CHECK(reservoir.is_empty());
    CHECK_FALSE(reservoir.is_full());
    CHECK(reservoir.invariant_holds());
}

TEST_REQ(FuelTank, robustesse_capacite_nulle, "LLR-M04-031") {
    FuelTank reservoir;
    CHECK_FALSE(FuelTank::create(Mass(), reservoir));
    // Le reservoir degenere reste neanmoins dans un etat coherent.
    CHECK(reservoir.invariant_holds());
    CHECK_NEAR(static_cast<double>(reservoir.fill_ratio_percent()), 0.0, 1e-6);
}

TEST_REQ(FuelTank, remplissage_partiel, "LLR-M04-032") {
    FuelTank reservoir;
    REQUIRE(FuelTank::create(grams(10000), reservoir));

    const Mass ajoute = reservoir.add(grams(2500));
    CHECK_EQ(ajoute.grams(), 2500);
    CHECK_EQ(reservoir.quantity().grams(), 2500);
    CHECK_NEAR(static_cast<double>(reservoir.fill_ratio_percent()), 25.0, 1e-4);
    CHECK(reservoir.invariant_holds());
}

TEST_REQ(FuelTank, debordement_a_l_ajout, "LLR-M04-033") {
    FuelTank reservoir;
    REQUIRE(FuelTank::create(grams(1000), reservoir));

    const Mass ajoute = reservoir.add(grams(5000));
    // Seule la place disponible est acceptee : l'invariant tient.
    CHECK_EQ(ajoute.grams(), 1000);
    CHECK_EQ(reservoir.quantity().grams(), 1000);
    CHECK(reservoir.is_full());
    CHECK(reservoir.invariant_holds());
}

TEST_REQ(FuelTank, prelevement_superieur_au_contenu, "LLR-M04-034") {
    FuelTank reservoir;
    REQUIRE(FuelTank::create(grams(1000), reservoir));
    (void)reservoir.add(grams(300));

    const Mass preleve = reservoir.remove(grams(900));
    CHECK_EQ(preleve.grams(), 300);
    CHECK(reservoir.is_empty());
    CHECK(reservoir.invariant_holds());
}

TEST_REQ(FuelTank, sequence_longue_invariant_toujours_vrai, "LLR-M04-035") {
    // Test de propriete : quelle que soit la sequence d'operations, y compris
    // absurde, l'invariant doit tenir. C'est ce type de test qui donne
    // confiance dans une classe a etat.
    FuelTank reservoir;
    REQUIRE(FuelTank::create(grams(5000), reservoir));

    const i32 operations[10] = {1000, -3000, 4000, 4000, -100, -100, 9000, -9000, 250, -250};
    for (i32 operation : operations) {
        if (operation >= 0) {
            (void)reservoir.add(grams(operation));
        } else {
            (void)reservoir.remove(grams(-operation));
        }
        REQUIRE(reservoir.invariant_holds());
        REQUIRE(reservoir.quantity() <= reservoir.capacity());
    }
}

// -----------------------------------------------------------------------------
//  Le type fort empeche la confusion d'unites
// -----------------------------------------------------------------------------
TEST_REQ(TypeFort, meme_masse_deux_unites, "LLR-M04-040") {
    // Le scenario du "Gimli Glider" : 22 300 unites de carburant.
    // Interpretees en livres au lieu de kilogrammes, cela fait moins de la
    // moitie de la masse attendue.
    Mass en_livres;
    Mass en_kilogrammes;
    REQUIRE(Mass::from_pounds(22300.0F, en_livres));
    REQUIRE(Mass::from_kilograms(22300.0F, en_kilogrammes));

    CHECK(en_livres < en_kilogrammes);
    CHECK_NEAR(static_cast<double>(en_livres.kilograms()), 10115.0, 1.0);
    CHECK_NEAR(static_cast<double>(en_kilogrammes.kilograms()), 22300.0, 1.0);

    // Une fois construites, les deux masses sont comparables SANS ambiguite :
    // l'unite a ete fixee a la frontiere du systeme, une seule fois.
    const Mass manquant = en_kilogrammes - en_livres;
    CHECK_NEAR(static_cast<double>(manquant.kilograms()), 12184.9, 1.0);
}
