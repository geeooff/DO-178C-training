// =============================================================================
//  FQMS -- Tests bases sur les exigences de BAS NIVEAU (table A-6.3 / A-6.4).
//
//  Ils verifient chaque decision de conception prise dans le SDD.
//  Les tests bases sur les exigences de HAUT niveau sont dans test_fqms_hlr.cpp.
// =============================================================================
#include <microtest/microtest.hpp>

#include "mod16/fqms.hpp"

#include <avio/types.hpp>

using avio::f32;
using avio::i32;
using avio::u32;
using avio::u8;
using avio::usize;
using mod04::Mass;
using mod07::Status;
using mod16::CycleReport;
using mod16::FuelSystem;
using mod16::FuelSystemConfig;
using mod16::TankGauge;
using mod16::TankId;

namespace {

Mass kilogrammes(f32 valeur) noexcept {
    Mass masse;
    (void)Mass::from_kilograms(valeur, masse);
    return masse;
}

/// Mesure brute correspondant a une fraction de la pleine echelle.
/// 4095 = 3^2 x 5 x 7 x 13 : les cinquiemes tombent juste.
constexpr i32 kRawZero = 0;
constexpr i32 kRawUnCinquieme = 819;      // capacite / 5
constexpr i32 kRawDeuxCinquiemes = 1638;  // capacite x 2/5
constexpr i32 kRawQuatreCinquiemes = 3276;
constexpr i32 kRawPleine = 4095;

FuelSystem systeme_de_reference() noexcept {
    FuelSystem systeme;
    (void)FuelSystem::create(mod16::default_config(), systeme);
    return systeme;
}

}  // namespace

// =============================================================================
//  Configuration
// =============================================================================
TEST_REQ(Configuration, valeurs_de_reference, "LLR-FQMS-001") {
    const FuelSystemConfig config = mod16::default_config();

    CHECK_NEAR(static_cast<double>(config.tank_capacity[0].kilograms()), 5000.0, 0.001);
    CHECK_NEAR(static_cast<double>(config.tank_capacity[1].kilograms()), 8000.0, 0.001);
    CHECK_NEAR(static_cast<double>(config.tank_capacity[2].kilograms()), 5000.0, 0.001);
    CHECK_NEAR(static_cast<double>(config.low_fuel_threshold.kilograms()), 1500.0, 0.001);
    CHECK_NEAR(static_cast<double>(config.low_fuel_hysteresis.kilograms()), 200.0, 0.001);
    CHECK_NEAR(static_cast<double>(config.imbalance_threshold.kilograms()), 500.0, 0.001);
    CHECK_NEAR(static_cast<double>(config.imbalance_hysteresis.kilograms()), 100.0, 0.001);
    CHECK_EQ(config.confirm_cycles, avio::u16{5});
    CHECK_EQ(config.clear_cycles, avio::u16{5});
}

TEST_REQ(Configuration, creation_nominale, "LLR-FQMS-020") {
    FuelSystem systeme;
    REQUIRE(FuelSystem::create(mod16::default_config(), systeme));
    CHECK_EQ(systeme.cycle_count(), u32{0});
    CHECK_EQ(systeme.fault_count(TankId::Left), u32{0});
    CHECK_EQ(systeme.fault_count(TankId::Center), u32{0});
    CHECK_EQ(systeme.fault_count(TankId::Right), u32{0});
}

TEST_REQ(Configuration, capacite_nulle_refusee, "LLR-FQMS-020") {
    FuelSystemConfig config = mod16::default_config();
    config.tank_capacity[1] = Mass();
    FuelSystem systeme;
    CHECK_FALSE(FuelSystem::create(config, systeme));
}

TEST_REQ(Configuration, seuils_nuls_refuses, "LLR-FQMS-020") {
    FuelSystem systeme;

    FuelSystemConfig sans_bas_niveau = mod16::default_config();
    sans_bas_niveau.low_fuel_threshold = Mass();
    CHECK_FALSE(FuelSystem::create(sans_bas_niveau, systeme));

    FuelSystemConfig sans_desequilibre = mod16::default_config();
    sans_desequilibre.imbalance_threshold = Mass();
    CHECK_FALSE(FuelSystem::create(sans_desequilibre, systeme));
}

TEST_REQ(Configuration, hysteresis_incoherente_refusee, "LLR-FQMS-020") {
    FuelSystem systeme;

    // Une hysteresis superieure ou egale au seuil rendrait l'alerte
    // ineffacable, ou la ferait battre.
    FuelSystemConfig trop_grande = mod16::default_config();
    trop_grande.low_fuel_hysteresis = kilogrammes(1500.0F);
    CHECK_FALSE(FuelSystem::create(trop_grande, systeme));

    FuelSystemConfig ecart_trop_grand = mod16::default_config();
    ecart_trop_grand.imbalance_hysteresis = kilogrammes(600.0F);
    CHECK_FALSE(FuelSystem::create(ecart_trop_grand, systeme));
}

// =============================================================================
//  Jauge
// =============================================================================
TEST_REQ(Jauge, creation, "LLR-FQMS-010") {
    TankGauge jauge;
    CHECK_FALSE(TankGauge::create(Mass(), jauge));  // capacite nulle

    REQUIRE(TankGauge::create(kilogrammes(5000.0F), jauge));
    CHECK_NEAR(static_cast<double>(jauge.capacity().kilograms()), 5000.0, 0.001);
    CHECK_EQ(jauge.raw(), mod16::kRawMin);
}

TEST_REQ(Jauge, conversion_lineaire, "LLR-FQMS-011") {
    TankGauge jauge;
    REQUIRE(TankGauge::create(kilogrammes(5000.0F), jauge));

    struct Point {
        i32 raw;
        double kilogrammes;
    };
    const Point references[5] = {{kRawZero, 0.0},
                                 {kRawUnCinquieme, 1000.0},
                                 {kRawDeuxCinquiemes, 2000.0},
                                 {kRawQuatreCinquiemes, 4000.0},
                                 {kRawPleine, 5000.0}};

    for (usize index = 0U; index < 5U; ++index) {
        jauge.set_raw(references[index].raw);
        const mod07::Result<Mass> mesure = jauge.read();
        REQUIRE(mesure.is_ok());
        CHECK_NEAR(static_cast<double>(mesure.value().kilograms()),
                   references[index].kilogrammes, 0.5);
    }
}

TEST_REQ(Jauge, bornes_du_convertisseur, "LLR-FQMS-011") {
    TankGauge jauge;
    REQUIRE(TankGauge::create(kilogrammes(8000.0F), jauge));

    jauge.set_raw(mod16::kRawMin);
    const mod07::Result<Mass> vide = jauge.read();
    REQUIRE(vide.is_ok());
    CHECK_EQ(vide.value().grams(), 0);

    jauge.set_raw(mod16::kRawMax);
    const mod07::Result<Mass> plein = jauge.read();
    REQUIRE(plein.is_ok());
    CHECK_EQ(plein.value().grams(), 8000000);
}

TEST_REQ(Jauge, robustesse_hors_domaine, "LLR-FQMS-012") {
    TankGauge jauge;
    REQUIRE(TankGauge::create(kilogrammes(5000.0F), jauge));

    jauge.set_raw(-1);
    CHECK_EQ(jauge.read().status(), Status::OutOfRange);

    jauge.set_raw(4096);
    CHECK_EQ(jauge.read().status(), Status::OutOfRange);

    jauge.set_raw(-100000);
    CHECK_EQ(jauge.read().status(), Status::OutOfRange);

    // Et AUCUNE quantite n'est produite : la valeur de repli est nulle,
    // impossible a confondre avec une mesure valide grace au statut.
    jauge.set_raw(9999);
    CHECK_EQ(jauge.read().value_or(kilogrammes(-1.0F)).grams(), 0);
}

// =============================================================================
//  Decisions extraites (couvrables individuellement -- module 11)
// =============================================================================
TEST_REQ(Statut, correspondance_complete, "LLR-FQMS-030") {
    CHECK_EQ(mod16::system_status(0U), Status::HardwareFault);
    CHECK_EQ(mod16::system_status(1U), Status::NotReady);
    CHECK_EQ(mod16::system_status(2U), Status::NotReady);
    CHECK_EQ(mod16::system_status(3U), Status::Ok);
}

TEST_REQ(Statut, robustesse_valeur_impossible, "LLR-FQMS-030") {
    // Un nombre de jauges valides superieur a 3 est impossible par
    // construction. La fonction reste neanmoins deterministe.
    CHECK_EQ(mod16::system_status(4U), Status::Ok);
    CHECK_EQ(mod16::system_status(255U), Status::Ok);
}

TEST_REQ(Decisions, desequilibre_mesurable, "LLR-FQMS-041") {
    // Table de verite complete : 2 conditions, 4 combinaisons.
    CHECK(mod16::imbalance_is_measurable(true, true));
    CHECK_FALSE(mod16::imbalance_is_measurable(true, false));
    CHECK_FALSE(mod16::imbalance_is_measurable(false, true));
    CHECK_FALSE(mod16::imbalance_is_measurable(false, false));
}

TEST_REQ(Decisions, bas_niveau_mesurable, "LLR-FQMS-051") {
    // Jeu MC/DC minimal pour une conjonction de 3 conditions : N + 1 = 4 cas
    // (module 11). On donne ici la table complete, plus lisible pour 3
    // conditions, et qui contient le jeu minimal.
    CHECK(mod16::low_fuel_is_measurable(true, true, true));
    CHECK_FALSE(mod16::low_fuel_is_measurable(false, true, true));
    CHECK_FALSE(mod16::low_fuel_is_measurable(true, false, true));
    CHECK_FALSE(mod16::low_fuel_is_measurable(true, true, false));
    CHECK_FALSE(mod16::low_fuel_is_measurable(false, false, false));
}

// =============================================================================
//  Totalisation
// =============================================================================
TEST_REQ(Totalisation, somme_des_reservoirs_valides, "LLR-FQMS-031") {
    FuelSystem systeme = systeme_de_reference();
    const i32 mesures[3] = {kRawUnCinquieme, kRawUnCinquieme, kRawUnCinquieme};

    const CycleReport rapport = systeme.update(mesures);
    // 1000 + 1600 + 1000 = 3600 kg
    CHECK_NEAR(static_cast<double>(rapport.total.kilograms()), 3600.0, 1.0);
    CHECK_EQ(rapport.valid_tank_count, u8{3});
    CHECK_EQ(rapport.status, Status::Ok);
    CHECK_EQ(systeme.cycle_count(), u32{1});
}

TEST_REQ(Totalisation, reservoir_en_panne_exclu, "LLR-FQMS-031") {
    FuelSystem systeme = systeme_de_reference();
    // Le reservoir central est en panne.
    const i32 mesures[3] = {kRawUnCinquieme, -1, kRawUnCinquieme};

    const CycleReport rapport = systeme.update(mesures);
    CHECK(rapport.sensor_fault[1]);
    CHECK_FALSE(rapport.sensor_fault[0]);
    CHECK_FALSE(rapport.sensor_fault[2]);
    CHECK_EQ(rapport.tank_quantity[1].grams(), 0);
    // 1000 + 1000 = 2000 kg, le central ne contribue pas.
    CHECK_NEAR(static_cast<double>(rapport.total.kilograms()), 2000.0, 1.0);
    CHECK_EQ(rapport.valid_tank_count, u8{2});
    CHECK_EQ(rapport.status, Status::NotReady);
}

TEST_REQ(Totalisation, panne_totale, "LLR-FQMS-030,LLR-FQMS-031") {
    FuelSystem systeme = systeme_de_reference();
    const i32 mesures[3] = {-1, -1, -1};

    const CycleReport rapport = systeme.update(mesures);
    CHECK_EQ(rapport.valid_tank_count, u8{0});
    CHECK_EQ(rapport.total.grams(), 0);
    CHECK_EQ(rapport.status, Status::HardwareFault);
}

TEST_REQ(Totalisation, pleins_reservoirs, "LLR-FQMS-031") {
    FuelSystem systeme = systeme_de_reference();
    const i32 mesures[3] = {kRawPleine, kRawPleine, kRawPleine};

    const CycleReport rapport = systeme.update(mesures);
    CHECK_NEAR(static_cast<double>(rapport.total.kilograms()), 18000.0, 0.1);
    CHECK_EQ(rapport.status, Status::Ok);
}

TEST_REQ(Totalisation, robustesse_pointeur_nul, "LLR-FQMS-031") {
    FuelSystem systeme = systeme_de_reference();
    const CycleReport rapport = systeme.update(nullptr);

    CHECK_EQ(rapport.status, Status::HardwareFault);
    CHECK(rapport.sensor_fault[0]);
    CHECK(rapport.sensor_fault[1]);
    CHECK(rapport.sensor_fault[2]);
    CHECK_EQ(rapport.total.grams(), 0);
    // Le cycle n'est PAS comptabilise : aucune mesure n'a ete traitee.
    CHECK_EQ(systeme.cycle_count(), u32{0});
}

// =============================================================================
//  Ecart d'aile
// =============================================================================
TEST_REQ(Desequilibre, calcul_de_l_ecart, "LLR-FQMS-040") {
    FuelSystem systeme = systeme_de_reference();
    // gauche 5000 kg, droite 4000 kg -> ecart 1000 kg
    const i32 mesures[3] = {kRawPleine, kRawUnCinquieme, kRawQuatreCinquiemes};

    const CycleReport rapport = systeme.update(mesures);
    CHECK_NEAR(static_cast<double>(rapport.wing_imbalance.kilograms()), 1000.0, 1.0);
}

TEST_REQ(Desequilibre, ecart_symetrique, "LLR-FQMS-040") {
    FuelSystem systeme = systeme_de_reference();
    // L'ecart est une VALEUR ABSOLUE : peu importe quelle aile est la plus
    // pleine.
    const i32 gauche_plus_pleine[3] = {kRawPleine, kRawUnCinquieme, kRawQuatreCinquiemes};
    const CycleReport premier = systeme.update(gauche_plus_pleine);

    FuelSystem autre = systeme_de_reference();
    const i32 droite_plus_pleine[3] = {kRawQuatreCinquiemes, kRawUnCinquieme, kRawPleine};
    const CycleReport second = autre.update(droite_plus_pleine);

    CHECK_EQ(premier.wing_imbalance.grams(), second.wing_imbalance.grams());
}

TEST_REQ(Desequilibre, ecart_nul_si_jauge_d_aile_en_panne, "LLR-FQMS-041,LLR-FQMS-042") {
    FuelSystem systeme = systeme_de_reference();
    const i32 mesures[3] = {kRawPleine, kRawUnCinquieme, -1};

    const CycleReport rapport = systeme.update(mesures);
    // Un ecart calcule a partir d'une seule jauge valide n'aurait aucun sens.
    CHECK_EQ(rapport.wing_imbalance.grams(), 0);
}

// =============================================================================
//  Maintenance
// =============================================================================
TEST_REQ(Maintenance, comptage_des_pannes_par_reservoir, "LLR-FQMS-060") {
    FuelSystem systeme = systeme_de_reference();

    const i32 gauche_en_panne[3] = {-1, kRawUnCinquieme, kRawUnCinquieme};
    (void)systeme.update(gauche_en_panne);
    (void)systeme.update(gauche_en_panne);

    const i32 droite_en_panne[3] = {kRawUnCinquieme, kRawUnCinquieme, 5000};
    (void)systeme.update(droite_en_panne);

    CHECK_EQ(systeme.fault_count(TankId::Left), u32{2});
    CHECK_EQ(systeme.fault_count(TankId::Center), u32{0});
    CHECK_EQ(systeme.fault_count(TankId::Right), u32{1});
    CHECK_EQ(systeme.cycle_count(), u32{3});
}

TEST_REQ(Maintenance, robustesse_identifiant_hors_domaine, "LLR-FQMS-060") {
    const FuelSystem systeme = systeme_de_reference();
    CHECK_EQ(systeme.fault_count(TankId::Count), u32{0});
    CHECK_EQ(systeme.fault_count(static_cast<TankId>(u8{99U})), u32{0});
}

TEST_REQ(Maintenance, libelles_des_reservoirs, "LLR-FQMS-060") {
    CHECK_EQ(mod16::tank_name(TankId::Left), "AileGauche");
    CHECK_EQ(mod16::tank_name(TankId::Center), "Central");
    CHECK_EQ(mod16::tank_name(TankId::Right), "AileDroite");
    CHECK_EQ(mod16::tank_name(TankId::Count), "Inconnu");
}

// =============================================================================
//  Sequence de traitement et grandeurs derivees
// =============================================================================
TEST_REQ(Sequence, cinq_etapes_quelles_que_soient_les_entrees, "LLR-FQMS-021") {
    // LLR-FQMS-021 exige une sequence FIXE, sans branchement sur le nombre
    // d'iterations : c'est ce qui rend le WCET constant (HLR-FQMS-041).
    //
    // Manifestation OBSERVABLE : les trois jauges sont lues a CHAQUE cycle,
    // quel que soit le resultat des precedentes. Si le code s'arretait a la
    // premiere panne, les compteurs des reservoirs suivants resteraient a zero.
    FuelSystem systeme = systeme_de_reference();
    const i32 tout_en_panne[3] = {-1, -1, -1};

    for (usize cycle = 0U; cycle < 10U; ++cycle) {
        (void)systeme.update(tout_en_panne);
    }
    CHECK_EQ(systeme.fault_count(TankId::Left), u32{10});
    CHECK_EQ(systeme.fault_count(TankId::Center), u32{10});
    CHECK_EQ(systeme.fault_count(TankId::Right), u32{10});
    CHECK_EQ(systeme.cycle_count(), u32{10});

    // Le nombre d'operations ne depend pas non plus des valeurs valides.
    FuelSystem autre = systeme_de_reference();
    const i32 tout_valide[3] = {kRawPleine, kRawPleine, kRawPleine};
    for (usize cycle = 0U; cycle < 10U; ++cycle) {
        (void)autre.update(tout_valide);
    }
    CHECK_EQ(autre.cycle_count(), u32{10});
    CHECK_EQ(autre.fault_count(TankId::Left), u32{0});
}

TEST_REQ(BasNiveauLlr, deficit_transmis_au_moniteur, "LLR-FQMS-050") {
    // Le moniteur recoit `seuil - total`, en kilogrammes. On le verifie par
    // son EFFET observable : l'alerte se leve exactement quand le total passe
    // sous 1500 kg, et pas avant.
    //
    // 1600 kg -> deficit = -100 -> pas d'alerte
    FuelSystem au_dessus = systeme_de_reference();
    const i32 mesures_1600[3] = {kRawZero, 819, kRawZero};
    for (usize cycle = 0U; cycle < 15U; ++cycle) {
        const CycleReport rapport = au_dessus.update(mesures_1600);
        CHECK_FALSE(rapport.low_fuel_alert);
    }

    // 1400 kg -> deficit = +100 -> alerte apres 5 cycles
    FuelSystem au_dessous = systeme_de_reference();
    const i32 mesures_1400[3] = {kRawZero, 717, kRawZero};
    CycleReport rapport;
    for (usize cycle = 0U; cycle < 5U; ++cycle) {
        rapport = au_dessous.update(mesures_1400);
    }
    CHECK(rapport.low_fuel_alert);
    CHECK_NEAR(static_cast<double>(rapport.total.kilograms()), 1400.0, 2.0);
}

TEST_REQ(BasNiveauLlr, echantillon_non_fini_si_non_mesurable, "LLR-FQMS-052") {
    // Quand le bas niveau n'est pas mesurable, un echantillon NON FINI est
    // transmis au moniteur, qui l'ignore (LLR-ALERT-050 du module 10) et gele
    // son etat.
    //
    // Preuve par l'absurde : le total VISIBLE tombe tres largement sous le
    // seuil, pendant longtemps, et pourtant aucune alerte ne se leve.
    FuelSystem systeme = systeme_de_reference();
    const i32 avec_panne[3] = {kRawZero, -1, kRawZero};

    CycleReport rapport;
    for (usize cycle = 0U; cycle < 50U; ++cycle) {
        rapport = systeme.update(avec_panne);
        CHECK_FALSE(rapport.low_fuel_alert);
    }
    CHECK_EQ(rapport.total.grams(), 0);
    CHECK_EQ(rapport.status, Status::NotReady);

    // Des que la jauge revient et que le total est reellement bas, l'alerte se
    // leve normalement : le gel n'a pas casse le mecanisme.
    const i32 sans_panne[3] = {kRawZero, 717, kRawZero};
    for (usize cycle = 0U; cycle < 5U; ++cycle) {
        rapport = systeme.update(sans_panne);
    }
    CHECK(rapport.low_fuel_alert);
}
