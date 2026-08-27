// =============================================================================
//  FQMS -- Tests bases sur les exigences de HAUT NIVEAU (table A-6.1 / A-6.2).
//
//  Vue "boite noire" : on n'utilise que l'interface publique du systeme et on
//  raisonne en termes de comportement observable par l'equipage. Aucun de ces
//  tests ne suppose de connaitre l'implementation.
//
//  En projet reel, ces tests seraient ecrits par une personne DIFFERENTE de
//  l'auteur du code (independance exigee en DAL B, table A-6).
// =============================================================================
#include <avio/types.hpp>
#include <microtest/microtest.hpp>

#include "mod16/fqms.hpp"

using avio::f32;
using avio::i32;
using avio::u32;
using avio::u8;
using avio::usize;
using mod07::Status;
using mod16::CycleReport;
using mod16::FuelSystem;
using mod16::TankId;

namespace {

// --- Mesures brutes utiles ----------------------------------------------------
constexpr i32 kVide = 0;
constexpr i32 kPlein = 4095;
constexpr i32 kPanne = -1;

/// Aile : 5000 kg pleine echelle.
constexpr i32 kAile4000kg = 3276;  // 4095 x 4/5
constexpr i32 kAile4550kg = 3726;
constexpr i32 kAile4700kg = 3849;

/// Central : 8000 kg pleine echelle.
constexpr i32 kCentral1400kg = 717;
constexpr i32 kCentral1600kg = 819;  // 4095 / 5
constexpr i32 kCentral1800kg = 921;

FuelSystem systeme_neuf() noexcept {
    FuelSystem systeme;
    (void)FuelSystem::create(mod16::default_config(), systeme);
    return systeme;
}

/// Applique le meme jeu de mesures pendant `cycles` cycles et renvoie le
/// dernier compte rendu.
CycleReport maintenir(FuelSystem& systeme, i32 gauche, i32 central, i32 droite,
                      usize cycles) noexcept {
    CycleReport rapport;
    const i32 mesures[3] = {gauche, central, droite};
    for (usize cycle = 0U; cycle < cycles; ++cycle) {
        rapport = systeme.update(mesures);
    }
    return rapport;
}

}  // namespace

// =============================================================================
//  HLR-FQMS-042 : etat au demarrage
// =============================================================================
TEST_REQ(Demarrage, aucune_alerte_a_la_mise_sous_tension, "HLR-FQMS-042") {
    FuelSystem systeme = systeme_neuf();

    // Meme avec des reservoirs quasi vides, aucune alerte au premier cycle :
    // la confirmation exige 5 cycles consecutifs. Une alerte active au
    // demarrage serait interpretee comme une panne reelle par l'equipage.
    const i32 mesures[3] = {kVide, kVide, kVide};
    const CycleReport premier = systeme.update(mesures);

    CHECK_FALSE(premier.low_fuel_alert);
    CHECK_FALSE(premier.imbalance_alert);
}

// =============================================================================
//  HLR-FQMS-001 / 010 / 011 : quantite et statut
// =============================================================================
TEST_REQ(Quantite, pleins_reservoirs, "HLR-FQMS-001,HLR-FQMS-010") {
    FuelSystem systeme = systeme_neuf();
    const CycleReport rapport = maintenir(systeme, kPlein, kPlein, kPlein, 1U);

    CHECK_NEAR(static_cast<double>(rapport.total.kilograms()), 18000.0, 1.0);
    CHECK_NEAR(static_cast<double>(rapport.tank_quantity[0].kilograms()), 5000.0, 1.0);
    CHECK_NEAR(static_cast<double>(rapport.tank_quantity[1].kilograms()), 8000.0, 1.0);
    CHECK_NEAR(static_cast<double>(rapport.tank_quantity[2].kilograms()), 5000.0, 1.0);
    CHECK_EQ(rapport.status, Status::Ok);
}

TEST_REQ(Quantite, statut_degrade_sur_panne_partielle, "HLR-FQMS-011") {
    FuelSystem systeme = systeme_neuf();
    const CycleReport une_panne = maintenir(systeme, kPlein, kPanne, kPlein, 1U);
    CHECK_EQ(une_panne.status, Status::NotReady);
    CHECK_EQ(une_panne.valid_tank_count, u8{2});

    FuelSystem autre = systeme_neuf();
    const CycleReport deux_pannes = maintenir(autre, kPanne, kPanne, kPlein, 1U);
    CHECK_EQ(deux_pannes.status, Status::NotReady);
    CHECK_EQ(deux_pannes.valid_tank_count, u8{1});
}

TEST_REQ(Quantite, statut_indisponible_sur_panne_totale, "HLR-FQMS-011") {
    FuelSystem systeme = systeme_neuf();
    const CycleReport rapport = maintenir(systeme, kPanne, kPanne, kPanne, 1U);
    CHECK_EQ(rapport.status, Status::HardwareFault);
    CHECK_EQ(rapport.total.grams(), 0);
}

TEST_REQ(Quantite, mesure_hors_domaine_rejetee, "HLR-FQMS-002") {
    FuelSystem systeme = systeme_neuf();
    const CycleReport rapport = maintenir(systeme, -5, 999999, kPlein, 1U);

    CHECK(rapport.sensor_fault[0]);
    CHECK(rapport.sensor_fault[1]);
    CHECK_FALSE(rapport.sensor_fault[2]);
    // Aucune quantite n'est produite pour les reservoirs en panne.
    CHECK_EQ(rapport.tank_quantity[0].grams(), 0);
    CHECK_EQ(rapport.tank_quantity[1].grams(), 0);
}

// =============================================================================
//  HLR-FQMS-030 / 031 : alerte bas niveau
// =============================================================================
TEST_REQ(BasNiveau, confirmation_apres_cinq_cycles, "HLR-FQMS-030") {
    FuelSystem systeme = systeme_neuf();
    const i32 mesures[3] = {kVide, kCentral1400kg, kVide};

    // 1400 kg < 1500 kg : la condition est remplie, mais l'alerte ne doit se
    // lever qu'apres 5 cycles CONSECUTIFS.
    for (usize cycle = 1U; cycle <= 4U; ++cycle) {
        const CycleReport rapport = systeme.update(mesures);
        CHECK_FALSE(rapport.low_fuel_alert);
    }
    const CycleReport cinquieme = systeme.update(mesures);
    CHECK(cinquieme.low_fuel_alert);
}

TEST_REQ(BasNiveau, pas_d_alerte_au_dessus_du_seuil, "HLR-FQMS-030") {
    FuelSystem systeme = systeme_neuf();
    // 1600 kg > 1500 kg : aucune alerte, meme apres de nombreux cycles.
    const CycleReport rapport = maintenir(systeme, kVide, kCentral1600kg, kVide, 20U);
    CHECK_NEAR(static_cast<double>(rapport.total.kilograms()), 1600.0, 1.0);
    CHECK_FALSE(rapport.low_fuel_alert);
}

TEST_REQ(BasNiveau, hysteresis_a_l_effacement, "HLR-FQMS-031") {
    FuelSystem systeme = systeme_neuf();

    // 1. L'alerte se leve.
    CycleReport rapport = maintenir(systeme, kVide, kCentral1400kg, kVide, 5U);
    REQUIRE(rapport.low_fuel_alert);

    // 2. Remontee a 1600 kg : au-dessus du seuil de 1500, mais SOUS le seuil
    //    d'effacement de 1700. L'alerte doit se MAINTENIR.
    rapport = maintenir(systeme, kVide, kCentral1600kg, kVide, 20U);
    CHECK(rapport.low_fuel_alert);

    // 3. Remontee a 1800 kg : au-dessus de 1700. L'alerte s'efface apres
    //    5 cycles.
    rapport = maintenir(systeme, kVide, kCentral1800kg, kVide, 4U);
    CHECK(rapport.low_fuel_alert);
    rapport = maintenir(systeme, kVide, kCentral1800kg, kVide, 1U);
    CHECK_FALSE(rapport.low_fuel_alert);
}

TEST_REQ(BasNiveau, bruit_ne_leve_pas_l_alerte, "HLR-FQMS-030") {
    // Ballottement du carburant en turbulence : la quantite oscille de part et
    // d'autre du seuil. L'anti-rebond doit filtrer.
    FuelSystem systeme = systeme_neuf();
    const i32 bas[3] = {kVide, kCentral1400kg, kVide};
    const i32 haut[3] = {kVide, kCentral1600kg, kVide};

    CycleReport rapport;
    for (usize cycle = 0U; cycle < 20U; ++cycle) {
        rapport = systeme.update(((cycle % 2U) == 0U) ? bas : haut);
        CHECK_FALSE(rapport.low_fuel_alert);
    }
}

TEST_REQ(BasNiveau, alerte_gelee_sur_panne, "HLR-FQMS-032") {
    FuelSystem systeme = systeme_neuf();

    // 1. Reservoirs pleins : aucune alerte.
    CycleReport rapport = maintenir(systeme, kPlein, kPlein, kPlein, 10U);
    REQUIRE_EQ(rapport.low_fuel_alert, false);

    // 2. La jauge centrale tombe en panne. La quantite VISIBLE chute a
    //    10 000 kg, mais l'avion n'a rien perdu : c'est la MESURE qui manque.
    //    L'alerte bas niveau ne doit PAS se lever.
    rapport = maintenir(systeme, kPlein, kPanne, kPlein, 20U);
    CHECK_FALSE(rapport.low_fuel_alert);
    CHECK_EQ(rapport.status, Status::NotReady);

    // Sans le gel, la quantite partielle (10 000 kg) resterait au-dessus du
    // seuil ici. Le test suivant montre le cas ou cela ferait vraiment mal.
}

TEST_REQ(BasNiveau, panne_ne_declenche_pas_de_fausse_alerte, "HLR-FQMS-032") {
    FuelSystem systeme = systeme_neuf();

    // 3000 kg au total, dont 2400 dans le central : au-dessus du seuil.
    const CycleReport nominal = maintenir(systeme, 246, kCentral1600kg, 246, 10U);
    CHECK_FALSE(nominal.low_fuel_alert);

    // La jauge centrale tombe en panne : la quantite VISIBLE tombe sous
    // 1500 kg. Sans le gel, une alerte bas niveau INJUSTIFIEE se leverait, et
    // l'equipage se derouterait sans raison.
    const CycleReport degrade = maintenir(systeme, 246, kPanne, 246, 20U);
    // La quantite VISIBLE est bien tombee sous le seuil de 1500 kg...
    CHECK(degrade.total.kilograms() < 1500.0F);
    // ... et pourtant AUCUNE alerte : le gel a joue son role.
    CHECK_FALSE(degrade.low_fuel_alert);
    CHECK_EQ(degrade.status, Status::NotReady);
}

// =============================================================================
//  HLR-FQMS-020 / 021 / 022 : alerte de desequilibre
// =============================================================================
TEST_REQ(Desequilibre, confirmation_apres_cinq_cycles, "HLR-FQMS-020") {
    FuelSystem systeme = systeme_neuf();
    const i32 mesures[3] = {kPlein, kPlein, kAile4000kg};  // ecart 1000 kg

    for (usize cycle = 1U; cycle <= 4U; ++cycle) {
        const CycleReport rapport = systeme.update(mesures);
        CHECK_FALSE(rapport.imbalance_alert);
    }
    const CycleReport cinquieme = systeme.update(mesures);
    CHECK(cinquieme.imbalance_alert);
    CHECK_NEAR(static_cast<double>(cinquieme.wing_imbalance.kilograms()), 1000.0, 1.0);
}

TEST_REQ(Desequilibre, pas_d_alerte_sous_le_seuil, "HLR-FQMS-020") {
    FuelSystem systeme = systeme_neuf();
    // ecart 450 kg < 500 kg
    const CycleReport rapport = maintenir(systeme, kPlein, kPlein, kAile4550kg, 20U);
    CHECK_NEAR(static_cast<double>(rapport.wing_imbalance.kilograms()), 450.0, 2.0);
    CHECK_FALSE(rapport.imbalance_alert);
}

TEST_REQ(Desequilibre, hysteresis_a_l_effacement, "HLR-FQMS-021") {
    FuelSystem systeme = systeme_neuf();

    // 1. Ecart de 1000 kg : l'alerte se leve.
    CycleReport rapport = maintenir(systeme, kPlein, kPlein, kAile4000kg, 5U);
    REQUIRE(rapport.imbalance_alert);

    // 2. Ecart ramene a 450 kg : sous le seuil de 500, mais AU-DESSUS du seuil
    //    d'effacement de 400. Zone morte : l'alerte se MAINTIENT.
    rapport = maintenir(systeme, kPlein, kPlein, kAile4550kg, 20U);
    CHECK(rapport.imbalance_alert);

    // 3. Ecart ramene a 300 kg : sous 400. L'alerte s'efface apres 5 cycles.
    rapport = maintenir(systeme, kPlein, kPlein, kAile4700kg, 4U);
    CHECK(rapport.imbalance_alert);
    rapport = maintenir(systeme, kPlein, kPlein, kAile4700kg, 1U);
    CHECK_FALSE(rapport.imbalance_alert);
}

TEST_REQ(Desequilibre, alerte_gelee_sur_panne_d_aile, "HLR-FQMS-022") {
    FuelSystem systeme = systeme_neuf();

    // 1. L'alerte de desequilibre se leve.
    CycleReport rapport = maintenir(systeme, kPlein, kPlein, kAile4000kg, 5U);
    REQUIRE(rapport.imbalance_alert);

    // 2. La jauge d'aile droite tombe en panne. L'ecart n'est plus mesurable :
    //    l'alerte doit etre CONSERVEE, ni levee ni effacee.
    rapport = maintenir(systeme, kPlein, kPlein, kPanne, 30U);
    CHECK(rapport.imbalance_alert);
    CHECK_EQ(rapport.wing_imbalance.grams(), 0);

    // 3. La jauge revient, ailes equilibrees : l'alerte s'efface normalement.
    rapport = maintenir(systeme, kPlein, kPlein, kPlein, 5U);
    CHECK_FALSE(rapport.imbalance_alert);
}

TEST_REQ(Desequilibre, panne_ne_leve_pas_de_fausse_alerte, "HLR-FQMS-022") {
    FuelSystem systeme = systeme_neuf();

    // Ailes equilibrees, aucune alerte.
    CycleReport rapport = maintenir(systeme, kPlein, kPlein, kPlein, 10U);
    REQUIRE_EQ(rapport.imbalance_alert, false);

    // La jauge droite tombe en panne : sans le gel, l'ecart calcule vaudrait
    // 5000 kg (gauche pleine, droite a zero) et l'alerte se leverait a tort.
    rapport = maintenir(systeme, kPlein, kPlein, kPanne, 30U);
    CHECK_FALSE(rapport.imbalance_alert);
}

// =============================================================================
//  HLR-FQMS-040 : maintenance
// =============================================================================
TEST_REQ(Maintenance, comptage_cumule_des_rejets, "HLR-FQMS-040") {
    FuelSystem systeme = systeme_neuf();
    (void)maintenir(systeme, kPanne, kPlein, kPlein, 7U);
    CHECK_EQ(systeme.fault_count(TankId::Left), u32{7});
    CHECK_EQ(systeme.fault_count(TankId::Center), u32{0});
    CHECK_EQ(systeme.fault_count(TankId::Right), u32{0});
    CHECK_EQ(systeme.cycle_count(), u32{7});
}

// =============================================================================
//  Scenario de vol complet
// =============================================================================
TEST_REQ(Vol, profil_complet, "HLR-FQMS-010,HLR-FQMS-030,HLR-FQMS-020") {
    FuelSystem systeme = systeme_neuf();

    // Decollage : pleins reservoirs.
    CycleReport rapport = maintenir(systeme, kPlein, kPlein, kPlein, 10U);
    CHECK_NEAR(static_cast<double>(rapport.total.kilograms()), 18000.0, 1.0);
    CHECK_FALSE(rapport.low_fuel_alert);
    CHECK_FALSE(rapport.imbalance_alert);

    // Croisiere : le central se vide en premier (sequence de consommation
    // reelle sur un biréacteur).
    rapport = maintenir(systeme, kPlein, kCentral1600kg, kPlein, 10U);
    CHECK_NEAR(static_cast<double>(rapport.total.kilograms()), 11600.0, 2.0);
    CHECK_FALSE(rapport.low_fuel_alert);

    // Un transfert dissymetrique cree un desequilibre.
    rapport = maintenir(systeme, kPlein, kVide, kAile4000kg, 10U);
    CHECK(rapport.imbalance_alert);

    // L'equipage retablit l'equilibre.
    rapport = maintenir(systeme, kAile4000kg, kVide, kAile4000kg, 10U);
    CHECK_FALSE(rapport.imbalance_alert);

    // Fin de vol : approche avec le carburant de reserve.
    rapport = maintenir(systeme, kVide, kCentral1400kg, kVide, 10U);
    CHECK(rapport.low_fuel_alert);
    CHECK_EQ(rapport.status, Status::Ok);
}

// =============================================================================
//  Coherence des unites de bout en bout (module 12, matrice D1..D5)
// =============================================================================
TEST_REQ(Integration, coherence_des_unites, "HLR-FQMS-010") {
    // La chaine traverse deux unites : gramme (interne) et kilogramme
    // (interfaces des moniteurs). C'est le point le plus dangereux du systeme
    // (module 04, Mars Climate Orbiter). On le verifie explicitement.
    FuelSystem systeme = systeme_neuf();
    const CycleReport rapport = maintenir(systeme, kPlein, kPlein, kPlein, 1U);

    const avio::i32 somme_grammes = rapport.tank_quantity[0].grams() +
                                    rapport.tank_quantity[1].grams() +
                                    rapport.tank_quantity[2].grams();
    CHECK_EQ(rapport.total.grams(), somme_grammes);
    CHECK_NEAR(static_cast<double>(rapport.total.kilograms()),
               static_cast<double>(somme_grammes) / 1000.0, 0.001);
}
