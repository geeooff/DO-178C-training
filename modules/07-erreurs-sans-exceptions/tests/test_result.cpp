#include <avio/assert.hpp>
#include <avio/types.hpp>
#include <microtest/microtest.hpp>

#include "mod07/arinc429.hpp"
#include "mod07/result.hpp"

using avio::i32;
using avio::u32;
using avio::u8;
using mod07::Arinc429Word;
using mod07::Result;
using mod07::SignStatus;
using mod07::Status;

namespace {

void handler_silencieux(const char* condition, const char* file, avio::i32 line) noexcept {
    avio::unused(condition, file, line);
}

}  // namespace

// =============================================================================
//  Status
// =============================================================================
TEST_REQ(Status, libelle_de_chaque_valeur, "LLR-M07-001") {
    CHECK_EQ(mod07::status_name(Status::Ok), "Ok");
    CHECK_EQ(mod07::status_name(Status::InvalidArgument), "ArgumentInvalide");
    CHECK_EQ(mod07::status_name(Status::OutOfRange), "HorsDomaine");
    CHECK_EQ(mod07::status_name(Status::ChecksumError), "ErreurIntegrite");
    CHECK_EQ(mod07::status_name(Status::NotReady), "NonDisponible");
    CHECK_EQ(mod07::status_name(Status::HardwareFault), "PanneMaterielle");
    CHECK_EQ(mod07::status_name(Status::Timeout), "Echeance");
}

TEST_REQ(Status, robustesse_valeur_hors_enumeration, "LLR-M07-002") {
    const Status fabrique = static_cast<Status>(u8{200U});
    CHECK_EQ(mod07::status_name(fabrique), "StatutInconnu");
    CHECK_FALSE(mod07::is_fault(fabrique));
}

TEST_REQ(Status, classification_des_pannes, "LLR-M07-003") {
    CHECK_FALSE(mod07::is_fault(Status::Ok));
    // NotReady est un etat TRANSITOIRE normal, pas une panne : distinction
    // essentielle, sous peine de declencher une alarme au demarrage.
    CHECK_FALSE(mod07::is_fault(Status::NotReady));
    CHECK_FALSE(mod07::is_fault(Status::InvalidArgument));

    CHECK(mod07::is_fault(Status::ChecksumError));
    CHECK(mod07::is_fault(Status::HardwareFault));
    CHECK(mod07::is_fault(Status::Timeout));
    CHECK(mod07::is_fault(Status::OutOfRange));
}

// =============================================================================
//  Result<T>
// =============================================================================
TEST_REQ(Result, defaut_est_une_erreur, "LLR-M07-010") {
    // Point de conception : un Result non initialise NE DOIT PAS passer pour
    // un succes. C'est la difference entre un defaut sur et un defaut discret.
    const Result<i32> resultat;
    CHECK_FALSE(resultat.is_ok());
    CHECK(resultat.is_error());
    CHECK_EQ(resultat.status(), Status::NotReady);
}

TEST_REQ(Result, succes, "LLR-M07-011") {
    const Result<i32> resultat = Result<i32>::ok(42);
    CHECK(resultat.is_ok());
    CHECK_EQ(resultat.status(), Status::Ok);
    CHECK_EQ(resultat.value(), 42);
    CHECK_EQ(resultat.value_or(-1), 42);
}

TEST_REQ(Result, erreur, "LLR-M07-012") {
    const Result<i32> resultat = Result<i32>::error(Status::Timeout);
    CHECK(resultat.is_error());
    CHECK_EQ(resultat.status(), Status::Timeout);
    CHECK_EQ(resultat.value_or(-1), -1);
}

TEST_REQ(Result, erreur_ok_est_neutralisee, "LLR-M07-013") {
    // Result::error(Status::Ok) serait un contresens : le type le refuse.
    const Result<i32> resultat = Result<i32>::error(Status::Ok);
    CHECK(resultat.is_error());
    CHECK_EQ(resultat.status(), Status::InvalidArgument);
}

TEST_REQ(Result, acces_value_sur_erreur_est_signale, "LLR-M07-014") {
    // Comportement de ROBUSTESSE : lire value() sur un Result en erreur est un
    // defaut d'utilisation. On le signale au gestionnaire d'anomalie ET on
    // renvoie une valeur deterministe -- jamais un comportement indefini.
    avio::reset_fault_count();
    const avio::FaultHandler precedent = avio::set_fault_handler(&handler_silencieux);

    const Result<i32> resultat = Result<i32>::error(Status::HardwareFault);
    const i32 valeur = resultat.value();

    avio::set_fault_handler(precedent);
    CHECK_EQ(avio::fault_count(), u32{1});
    CHECK_EQ(valeur, 0);
}

// =============================================================================
//  StatusCounters
// =============================================================================
TEST_REQ(Compteurs, comptage_et_dominante, "LLR-M07-020") {
    mod07::StatusCounters compteurs;
    compteurs.reset();

    compteurs.record(Status::Ok);
    compteurs.record(Status::ChecksumError);
    compteurs.record(Status::ChecksumError);
    compteurs.record(Status::ChecksumError);
    compteurs.record(Status::Timeout);
    compteurs.record(Status::NotReady);

    CHECK_EQ(compteurs.count(Status::Ok), u32{1});
    CHECK_EQ(compteurs.count(Status::ChecksumError), u32{3});
    CHECK_EQ(compteurs.count(Status::NotReady), u32{1});

    // NotReady et Ok ne sont pas des pannes : 3 + 1 = 4.
    CHECK_EQ(compteurs.total_faults(), u32{4});
    CHECK_EQ(compteurs.dominant_fault(), Status::ChecksumError);
}

TEST_REQ(Compteurs, etat_initial_et_remise_a_zero, "LLR-M07-021") {
    mod07::StatusCounters compteurs;
    compteurs.reset();
    CHECK_EQ(compteurs.total_faults(), u32{0});
    CHECK_EQ(compteurs.dominant_fault(), Status::Ok);

    compteurs.record(Status::HardwareFault);
    CHECK_EQ(compteurs.total_faults(), u32{1});
    compteurs.reset();
    CHECK_EQ(compteurs.total_faults(), u32{0});
}

TEST_REQ(Compteurs, robustesse_statut_hors_enumeration, "LLR-M07-022") {
    mod07::StatusCounters compteurs;
    compteurs.reset();
    compteurs.record(static_cast<Status>(u8{99U}));  // ne doit rien ecraser
    CHECK_EQ(compteurs.total_faults(), u32{0});
    CHECK_EQ(compteurs.count(static_cast<Status>(u8{99U})), u32{0});
}

// =============================================================================
//  ARINC 429
// =============================================================================
TEST_REQ(Arinc, parite_impaire, "LLR-M07-030") {
    CHECK(mod07::has_odd_parity(0x00000001U));
    CHECK_FALSE(mod07::has_odd_parity(0x00000003U));
    CHECK(mod07::has_odd_parity(0x00000007U));
    CHECK_FALSE(mod07::has_odd_parity(0x00000000U));
}

TEST_REQ(Arinc, encodage_puis_decodage, "LLR-M07-031") {
    const u32 brut = mod07::encode(mod07::kLabelAltitude, 1U, 35000U, SignStatus::NormalOperation);
    CHECK(mod07::has_odd_parity(brut));

    const Result<Arinc429Word> resultat = mod07::decode(brut);
    REQUIRE(resultat.is_ok());

    const Arinc429Word& mot = resultat.value();
    CHECK_EQ(mot.label, mod07::kLabelAltitude);
    CHECK_EQ(mot.sdi, u8{1U});
    CHECK_EQ(mot.payload, u32{35000U});
    CHECK_EQ(mot.ssm, SignStatus::NormalOperation);
    CHECK_EQ(mot.signed_value, 35000);
}

TEST_REQ(Arinc, valeur_negative_complement_a_deux, "LLR-M07-032") {
    // -500 pieds sur 19 bits : 2^19 - 500 = 523788
    const u32 charge = 524288U - 500U;
    const u32 brut = mod07::encode(mod07::kLabelAltitude, 0U, charge, SignStatus::NormalOperation);

    const Result<Arinc429Word> resultat = mod07::decode(brut);
    REQUIRE(resultat.is_ok());
    CHECK_EQ(resultat.value().signed_value, -500);
}

TEST_REQ(Arinc, robustesse_parite_alteree, "LLR-M07-033") {
    u32 brut = mod07::encode(mod07::kLabelAltitude, 0U, 1000U, SignStatus::NormalOperation);
    brut ^= 0x00001000U;  // un bit bascule pendant la transmission

    const Result<Arinc429Word> resultat = mod07::decode(brut);
    CHECK(resultat.is_error());
    CHECK_EQ(resultat.status(), Status::ChecksumError);
}

TEST_REQ(Arinc, robustesse_label_non_traite, "LLR-M07-034") {
    const u32 brut = mod07::encode(u8{42U}, 0U, 1000U, SignStatus::NormalOperation);
    const Result<Arinc429Word> resultat = mod07::decode(brut);
    CHECK(resultat.is_error());
    CHECK_EQ(resultat.status(), Status::InvalidArgument);
}

TEST_REQ(Arinc, ssm_panne_source, "LLR-M07-035") {
    const u32 brut = mod07::encode(mod07::kLabelAltitude, 0U, 1000U, SignStatus::FailureWarning);
    const Result<Arinc429Word> resultat = mod07::decode(brut);
    CHECK(resultat.is_error());
    CHECK_EQ(resultat.status(), Status::HardwareFault);
}

TEST_REQ(Arinc, ssm_donnee_indisponible, "LLR-M07-036") {
    const u32 sans_donnee =
        mod07::encode(mod07::kLabelAltitude, 0U, 1000U, SignStatus::NoComputedData);
    CHECK_EQ(mod07::decode(sans_donnee).status(), Status::NotReady);

    // Donnee de TEST FONCTIONNEL : valide techniquement, interdite en vol.
    const u32 test_fonctionnel =
        mod07::encode(mod07::kLabelAltitude, 0U, 1000U, SignStatus::FunctionalTest);
    CHECK_EQ(mod07::decode(test_fonctionnel).status(), Status::NotReady);
}

// -----------------------------------------------------------------------------
//  Chainage : extract_altitude_feet
// -----------------------------------------------------------------------------
TEST_REQ(Chainage, altitude_nominale, "LLR-M07-040") {
    const u32 brut = mod07::encode(mod07::kLabelAltitude, 0U, 12000U, SignStatus::NormalOperation);
    const Result<i32> resultat = mod07::extract_altitude_feet(brut);
    REQUIRE(resultat.is_ok());
    CHECK_EQ(resultat.value(), 12000);
}

TEST_REQ(Chainage, propagation_de_l_erreur_de_parite, "LLR-M07-041") {
    u32 brut = mod07::encode(mod07::kLabelAltitude, 0U, 12000U, SignStatus::NormalOperation);
    brut ^= 0x00000010U;
    // Le statut de l'etape la plus profonde remonte INTACT jusqu'a l'appelant.
    CHECK_EQ(mod07::extract_altitude_feet(brut).status(), Status::ChecksumError);
}

TEST_REQ(Chainage, mauvais_label, "LLR-M07-042") {
    const u32 brut = mod07::encode(mod07::kLabelAirspeed, 0U, 12000U, SignStatus::NormalOperation);
    CHECK_EQ(mod07::extract_altitude_feet(brut).status(), Status::InvalidArgument);
}

TEST_REQ(Chainage, valeur_hors_domaine_de_vol, "LLR-M07-043") {
    // Parite correcte, label correct, SSM correct... mais 200 000 pieds n'est
    // pas une altitude d'avion de ligne. La validation de DOMAINE reste
    // indispensable meme quand l'integrite du message est bonne.
    const u32 brut = mod07::encode(mod07::kLabelAltitude, 0U, 200000U, SignStatus::NormalOperation);
    CHECK_EQ(mod07::extract_altitude_feet(brut).status(), Status::OutOfRange);
}

TEST_REQ(Chainage, bornes_du_domaine_de_vol, "LLR-M07-044") {
    const u32 plafond =
        mod07::encode(mod07::kLabelAltitude, 0U, 60000U, SignStatus::NormalOperation);
    CHECK(mod07::extract_altitude_feet(plafond).is_ok());

    const u32 au_dessus =
        mod07::encode(mod07::kLabelAltitude, 0U, 60001U, SignStatus::NormalOperation);
    CHECK_EQ(mod07::extract_altitude_feet(au_dessus).status(), Status::OutOfRange);

    const u32 plancher =
        mod07::encode(mod07::kLabelAltitude, 0U, 524288U - 2000U, SignStatus::NormalOperation);
    CHECK(mod07::extract_altitude_feet(plancher).is_ok());

    const u32 en_dessous =
        mod07::encode(mod07::kLabelAltitude, 0U, 524288U - 2001U, SignStatus::NormalOperation);
    CHECK_EQ(mod07::extract_altitude_feet(en_dessous).status(), Status::OutOfRange);
}
