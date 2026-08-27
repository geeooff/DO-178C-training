// =============================================================================
//  Module 09 -- campagne de test BASEE SUR LES EXIGENCES.
//
//  Chaque cas de test est trace vers la ou les exigences qu'il verifie. Aucun
//  test "parce que ca me parait utile" : en DO-178C, un test sans exigence est
//  un test orphelin, et il apparait comme tel dans le rapport de tracabilite.
// =============================================================================
#include <avio/types.hpp>
#include <limits>
#include <microtest/microtest.hpp>

#include "mod09/altitude.hpp"

using avio::f32;
using avio::usize;
using mod07::Status;

namespace {

constexpr f32 kNaN = std::numeric_limits<f32>::quiet_NaN();
constexpr f32 kInf = std::numeric_limits<f32>::infinity();

/// Budget d'erreur alloue au calcul logiciel par HLR-ADCALT-004.
constexpr double kToleranceFeet = 20.0;

}  // namespace

// =============================================================================
//  Validation des entrees
// =============================================================================
TEST_REQ(Validation, pression_domaine_nominal, "LLR-ADCALT-010") {
    CHECK_EQ(mod09::validate_static_pressure(1013.25F), Status::Ok);
    CHECK_EQ(mod09::validate_static_pressure(500.0F), Status::Ok);
}

TEST_REQ(Validation, pression_bornes_incluses, "LLR-ADCALT-010") {
    // Les bornes sont INCLUSES (HLR-ADCALT-002 dit "inclus").
    CHECK_EQ(mod09::validate_static_pressure(mod09::kStaticPressureMinHpa), Status::Ok);
    CHECK_EQ(mod09::validate_static_pressure(mod09::kStaticPressureMaxHpa), Status::Ok);
}

TEST_REQ(Validation, pression_hors_domaine, "LLR-ADCALT-010") {
    CHECK_EQ(mod09::validate_static_pressure(99.9F), Status::OutOfRange);
    CHECK_EQ(mod09::validate_static_pressure(1100.1F), Status::OutOfRange);
    CHECK_EQ(mod09::validate_static_pressure(0.0F), Status::OutOfRange);
    CHECK_EQ(mod09::validate_static_pressure(-1.0F), Status::OutOfRange);
}

TEST_REQ(Validation, pression_non_finie, "LLR-ADCALT-010") {
    // Statut DISTINCT de OutOfRange : c'est l'exigence derivee HLR-ADCALT-007
    // qui l'impose, pour que la maintenance puisse distinguer un capteur
    // deregle d'un capteur en panne.
    CHECK_EQ(mod09::validate_static_pressure(kNaN), Status::InvalidArgument);
    CHECK_EQ(mod09::validate_static_pressure(kInf), Status::InvalidArgument);
    CHECK_EQ(mod09::validate_static_pressure(-kInf), Status::InvalidArgument);
}

TEST_REQ(Validation, qnh_domaine_et_bornes, "LLR-ADCALT-030") {
    CHECK_EQ(mod09::validate_qnh(1013.0F), Status::Ok);
    CHECK_EQ(mod09::validate_qnh(mod09::kQnhMinHpa), Status::Ok);
    CHECK_EQ(mod09::validate_qnh(mod09::kQnhMaxHpa), Status::Ok);
    CHECK_EQ(mod09::validate_qnh(947.9F), Status::OutOfRange);
    CHECK_EQ(mod09::validate_qnh(1084.1F), Status::OutOfRange);
    CHECK_EQ(mod09::validate_qnh(kNaN), Status::InvalidArgument);
}

// =============================================================================
//  Altitude-pression
// =============================================================================
TEST_REQ(Altitude, atmosphere_standard_donne_zero, "LLR-ADCALT-020") {
    const mod07::Result<f32> resultat = mod09::pressure_altitude_feet(1013.25F);
    REQUIRE(resultat.is_ok());
    CHECK_NEAR(static_cast<double>(resultat.value()), 0.0, kToleranceFeet);
}

TEST_REQ(Altitude, table_de_reference, "LLR-ADCALT-022") {
    // Valeurs de reference issues du modele ISA. Ce sont ces huit points qui
    // demontrent l'objectif HLR-ADCALT-004 (erreur <= 20 ft).
    struct Point {
        f32 pression_hpa;
        double altitude_ft;
    };
    const Point references[8] = {{1013.25F, 0.0},    {1000.0F, 363.64}, {950.0F, 1772.03},
                                 {850.0F, 4779.19},  {700.0F, 9878.39}, {500.0F, 18281.18},
                                 {300.0F, 30052.74}, {200.0F, 38615.05}};

    for (usize index = 0U; index < 8U; ++index) {
        const mod07::Result<f32> resultat =
            mod09::pressure_altitude_feet(references[index].pression_hpa);
        REQUIRE(resultat.is_ok());
        CHECK_NEAR(static_cast<double>(resultat.value()), references[index].altitude_ft,
                   kToleranceFeet);
    }
}

TEST_REQ(Altitude, pression_superieure_donne_altitude_negative, "LLR-ADCALT-020") {
    // Cas reel : une depression de 1050 hPa au sol donne une altitude-pression
    // negative. Ce n'est pas une anomalie.
    const mod07::Result<f32> resultat = mod09::pressure_altitude_feet(1050.0F);
    REQUIRE(resultat.is_ok());
    CHECK_NEAR(static_cast<double>(resultat.value()), -988.83, kToleranceFeet);
}

TEST_REQ(Altitude, monotonie, "LLR-ADCALT-023") {
    // Propriete structurelle : la fonction doit etre STRICTEMENT decroissante.
    // Un test de propriete complete utilement les points de reference : il
    // detecterait une inversion de signe qui passerait entre deux points.
    // Compteur ENTIER : une variable de boucle flottante accumulerait
    // l'erreur d'arrondi et le nombre d'iterations dependrait de la cible.
    double precedente = 1.0e9;
    for (avio::u32 pas = 0U; pas <= 100U; ++pas) {
        const f32 pression = mod09::kStaticPressureMinHpa + (static_cast<f32>(pas) * 10.0F);
        const mod07::Result<f32> resultat = mod09::pressure_altitude_feet(pression);
        REQUIRE(resultat.is_ok());
        const double altitude = static_cast<double>(resultat.value());
        REQUIRE(altitude < precedente);
        precedente = altitude;
    }
}

TEST_REQ(Altitude, robustesse_hors_domaine, "LLR-ADCALT-021") {
    CHECK_EQ(mod09::pressure_altitude_feet(50.0F).status(), Status::OutOfRange);
    CHECK_EQ(mod09::pressure_altitude_feet(2000.0F).status(), Status::OutOfRange);
}

TEST_REQ(Altitude, robustesse_non_finie, "LLR-ADCALT-021") {
    CHECK_EQ(mod09::pressure_altitude_feet(kNaN).status(), Status::InvalidArgument);
    CHECK_EQ(mod09::pressure_altitude_feet(kInf).status(), Status::InvalidArgument);
}

// =============================================================================
//  Correction du calage altimetrique
// =============================================================================
TEST_REQ(Correction, calage_standard_sans_effet, "LLR-ADCALT-031") {
    const mod07::Result<f32> avec = mod09::corrected_altitude_feet(850.0F, 1013.25F);
    const mod07::Result<f32> sans = mod09::pressure_altitude_feet(850.0F);
    REQUIRE(avec.is_ok());
    REQUIRE(sans.is_ok());
    CHECK_NEAR(static_cast<double>(avec.value()), static_cast<double>(sans.value()), 0.5);
}

TEST_REQ(Correction, calage_bas_abaisse_l_altitude, "LLR-ADCALT-031") {
    // QNH 1003,25 hPa = 10 hPa sous le standard -> -270 ft.
    const mod07::Result<f32> resultat = mod09::corrected_altitude_feet(850.0F, 1003.25F);
    REQUIRE(resultat.is_ok());
    CHECK_NEAR(static_cast<double>(resultat.value()), 4779.19 - 270.0, kToleranceFeet);
}

TEST_REQ(Correction, calage_haut_releve_l_altitude, "LLR-ADCALT-031") {
    const mod07::Result<f32> resultat = mod09::corrected_altitude_feet(850.0F, 1023.25F);
    REQUIRE(resultat.is_ok());
    CHECK_NEAR(static_cast<double>(resultat.value()), 4779.19 + 270.0, kToleranceFeet);
}

TEST_REQ(Correction, robustesse_calage_hors_domaine, "LLR-ADCALT-032") {
    CHECK_EQ(mod09::corrected_altitude_feet(850.0F, 900.0F).status(), Status::OutOfRange);
    CHECK_EQ(mod09::corrected_altitude_feet(850.0F, 1200.0F).status(), Status::OutOfRange);
    CHECK_EQ(mod09::corrected_altitude_feet(850.0F, kNaN).status(), Status::InvalidArgument);
}

TEST_REQ(Correction, priorite_des_erreurs, "LLR-ADCALT-032") {
    // LES DEUX entrees sont invalides. L'exigence SPECIFIE que c'est le statut
    // de la PRESSION qui remonte. Sans cette specification, le comportement
    // dependrait de l'implementation et ne serait pas verifiable.
    const mod07::Result<f32> resultat = mod09::corrected_altitude_feet(50.0F, 1200.0F);
    CHECK_EQ(resultat.status(), Status::OutOfRange);

    const mod07::Result<f32> nan_pression = mod09::corrected_altitude_feet(kNaN, 1200.0F);
    CHECK_EQ(nan_pression.status(), Status::InvalidArgument);
}

TEST_REQ(Correction, constante_exposee, "LLR-ADCALT-033") {
    CHECK_NEAR(static_cast<double>(mod09::feet_per_hpa()), 27.0, 1e-6);
}

// =============================================================================
//  Tests bases sur les exigences de HAUT NIVEAU
//
//  La table A-6 de la DO-178C demande des tests fondes sur les HLR, distincts
//  des tests fondes sur les LLR. Les premiers verifient le COMPORTEMENT ATTENDU
//  DU COMPOSANT, sans connaissance de son implementation ; les seconds
//  verifient chaque decision de conception.
//
//  En pratique : ces tests seraient ecrits par une personne DIFFERENTE de
//  l'auteur du code (independance exigee en DAL A et B, table A-6 colonne
//  "with independence").
// =============================================================================

TEST_REQ(SystemeADCALT, calcul_isa_conforme, "HLR-ADCALT-001") {
    // Vue "boite noire" : on ne verifie pas la formule, on verifie que le
    // composant se comporte comme l'atmosphere standard.
    const mod07::Result<f32> niveau_mer = mod09::pressure_altitude_feet(1013.25F);
    const mod07::Result<f32> niveau_croisiere = mod09::pressure_altitude_feet(238.4F);
    REQUIRE(niveau_mer.is_ok());
    REQUIRE(niveau_croisiere.is_ok());

    CHECK_NEAR(static_cast<double>(niveau_mer.value()), 0.0, kToleranceFeet);
    // 238,4 hPa correspond au niveau de vol 350 (35 000 ft), altitude de
    // croisiere typique d'un avion de ligne.
    CHECK_NEAR(static_cast<double>(niveau_croisiere.value()), 35000.0, 100.0);
}

TEST_REQ(SystemeADCALT, domaine_de_pression_accepte, "HLR-ADCALT-002") {
    // Le domaine COMPLET doit etre accepte, pas seulement quelques points.
    for (avio::u32 pas = 0U; pas <= 1000U; ++pas) {
        const f32 pression = 100.0F + static_cast<f32>(pas);
        REQUIRE(mod09::pressure_altitude_feet(pression).is_ok());
    }
}

TEST_REQ(SystemeADCALT, rejet_hors_domaine, "HLR-ADCALT-003") {
    const f32 invalides[6] = {0.0F, -1.0F, 99.99F, 1100.01F, kNaN, kInf};
    for (usize index = 0U; index < 6U; ++index) {
        const mod07::Result<f32> resultat = mod09::pressure_altitude_feet(invalides[index]);
        CHECK(resultat.is_error());
        // Aucune altitude ne doit etre produite : c'est le point de securite.
        CHECK_NEAR(static_cast<double>(resultat.value_or(-99999.0F)), -99999.0, 1e-3);
    }
}

TEST_REQ(SystemeADCALT, budget_d_erreur_respecte, "HLR-ADCALT-004") {
    // Verification du budget de 20 ft sur les huit points de reference du
    // modele ISA. C'est la demonstration de l'exigence, pas un echantillonnage
    // de confort.
    struct Point {
        f32 pression_hpa;
        double altitude_ft;
    };
    const Point references[8] = {{1013.25F, 0.0},    {1000.0F, 363.64}, {950.0F, 1772.03},
                                 {850.0F, 4779.19},  {700.0F, 9878.39}, {500.0F, 18281.18},
                                 {300.0F, 30052.74}, {200.0F, 38615.05}};
    double erreur_max = 0.0;
    for (usize index = 0U; index < 8U; ++index) {
        const mod07::Result<f32> resultat =
            mod09::pressure_altitude_feet(references[index].pression_hpa);
        REQUIRE(resultat.is_ok());
        const double ecart = static_cast<double>(resultat.value()) - references[index].altitude_ft;
        const double absolu = (ecart < 0.0) ? -ecart : ecart;
        if (absolu > erreur_max) {
            erreur_max = absolu;
        }
    }
    CHECK(erreur_max <= kToleranceFeet);
}

TEST_REQ(SystemeADCALT, calage_altimetrique_disponible, "HLR-ADCALT-005") {
    for (avio::u32 pas = 0U; pas <= 136U; ++pas) {
        const f32 calage = 948.0F + static_cast<f32>(pas);
        REQUIRE(mod09::corrected_altitude_feet(850.0F, calage).is_ok());
    }
}

TEST_REQ(SystemeADCALT, calage_hors_domaine_rejete, "HLR-ADCALT-006") {
    CHECK(mod09::corrected_altitude_feet(850.0F, 947.99F).is_error());
    CHECK(mod09::corrected_altitude_feet(850.0F, 1084.01F).is_error());
    CHECK(mod09::corrected_altitude_feet(850.0F, 0.0F).is_error());
}

TEST_REQ(SystemeADCALT, causes_de_rejet_distinctes, "HLR-ADCALT-007") {
    // L'exigence DERIVEE : les trois causes doivent etre DISCERNABLES.
    CHECK_EQ(mod09::pressure_altitude_feet(50.0F).status(), Status::OutOfRange);
    CHECK_EQ(mod09::pressure_altitude_feet(kNaN).status(), Status::InvalidArgument);
    CHECK_EQ(mod09::corrected_altitude_feet(850.0F, 900.0F).status(), Status::OutOfRange);

    // Sans cette distinction, la maintenance ne pourrait pas differencier un
    // capteur deregle (hors domaine) d'un capteur en panne (valeur non finie).
    CHECK(mod09::pressure_altitude_feet(50.0F).status() !=
          mod09::pressure_altitude_feet(kNaN).status());
}
