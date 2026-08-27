// =============================================================================
//  Module 01 -- tests bases sur les exigences.
//
//  Deux familles obligatoires en DO-178C (6.4.2) :
//    * cas NOMINAUX      : le domaine d'entree normal ;
//    * cas de ROBUSTESSE : les entrees anormales, les bornes, les valeurs
//                          impossibles selon la specification.
//  Les tests de robustesse sont ceux que l'on oublie... et ceux qui trouvent
//  les vrais defauts.
// =============================================================================
#include <avio/types.hpp>
#include <limits>
#include <microtest/microtest.hpp>

#include "mod01/safe_arith.hpp"

using avio::i16;
using avio::i32;
using avio::u16;
using avio::u8;

// -----------------------------------------------------------------------------
//  Enumerations
// -----------------------------------------------------------------------------
TEST_REQ(Enum, valeurs_nominales_valides, "LLR-M01-001") {
    CHECK(mod01::is_valid(mod01::SensorId::LeftPitot));
    CHECK(mod01::is_valid(mod01::SensorId::RightPitot));
    CHECK(mod01::is_valid(mod01::SensorId::StandbyPitot));
    CHECK(mod01::is_valid(mod01::SensorId::StaticPort));
}

TEST_REQ(Enum, robustesse_valeur_hors_domaine, "LLR-M01-002") {
    // Une valeur recue d'un bus peut etre n'importe quoi. Le cast la transforme
    // en SensorId sans aucun controle : c'est a nous de valider.
    const mod01::SensorId corrupted = static_cast<mod01::SensorId>(u8{200U});
    CHECK_FALSE(mod01::is_valid(corrupted));
    CHECK_EQ(mod01::name_of(corrupted), "Inconnu");
}

TEST_REQ(Enum, nom_de_chaque_membre, "LLR-M01-003") {
    CHECK_EQ(mod01::name_of(mod01::SensorId::LeftPitot), "PitotGauche");
    CHECK_EQ(mod01::name_of(mod01::SensorId::RightPitot), "PitotDroit");
    CHECK_EQ(mod01::name_of(mod01::SensorId::StandbyPitot), "PitotSecours");
    CHECK_EQ(mod01::name_of(mod01::SensorId::StaticPort), "PriseStatique");
    CHECK_EQ(mod01::name_of(mod01::SensorId::Count), "Inconnu");
}

TEST_REQ(Enum, type_sous_jacent_est_un_octet, "LLR-M01-004") {
    // La taille de l'enum fait partie du contrat d'interface : si elle change,
    // le format des messages echanges change aussi.
    static_assert(sizeof(mod01::SensorId) == 1U, "SensorId doit tenir sur 8 bits");
    CHECK_EQ(sizeof(mod01::SensorId), static_cast<avio::usize>(1));
    CHECK_EQ(mod01::to_underlying(mod01::SensorId::StaticPort), u8{3U});
}

// -----------------------------------------------------------------------------
//  Arithmetique saturante
// -----------------------------------------------------------------------------
TEST_REQ(Saturation, addition_domaine_nominal, "LLR-M01-010") {
    CHECK_EQ(mod01::saturating_add(i16{100}, i16{200}), i16{300});
    CHECK_EQ(mod01::saturating_add(i16{-100}, i16{50}), i16{-50});
    CHECK_EQ(mod01::saturating_add(i16{0}, i16{0}), i16{0});
}

TEST_REQ(Saturation, addition_aux_bornes, "LLR-M01-011") {
    // Analyse aux limites : on teste exactement la borne, puis juste au-dela.
    CHECK_EQ(mod01::saturating_add(mod01::kI16Max, i16{0}), mod01::kI16Max);
    CHECK_EQ(mod01::saturating_add(mod01::kI16Max, i16{1}), mod01::kI16Max);
    CHECK_EQ(mod01::saturating_add(mod01::kI16Max, mod01::kI16Max), mod01::kI16Max);
    CHECK_EQ(mod01::saturating_add(mod01::kI16Min, i16{-1}), mod01::kI16Min);
    CHECK_EQ(mod01::saturating_add(mod01::kI16Min, mod01::kI16Min), mod01::kI16Min);
}

TEST_REQ(Saturation, soustraction_aux_bornes, "LLR-M01-012") {
    CHECK_EQ(mod01::saturating_sub(i16{300}, i16{100}), i16{200});
    CHECK_EQ(mod01::saturating_sub(mod01::kI16Min, i16{1}), mod01::kI16Min);
    CHECK_EQ(mod01::saturating_sub(mod01::kI16Max, i16{-1}), mod01::kI16Max);
    // Piege classique : -kI16Min n'est pas representable sur 16 bits signes.
    CHECK_EQ(mod01::saturating_sub(i16{0}, mod01::kI16Min), mod01::kI16Max);
}

TEST_REQ(Saturation, multiplication_aux_bornes, "LLR-M01-013") {
    CHECK_EQ(mod01::saturating_mul(i16{100}, i16{3}), i16{300});
    CHECK_EQ(mod01::saturating_mul(i16{1000}, i16{1000}), mod01::kI16Max);
    CHECK_EQ(mod01::saturating_mul(i16{-1000}, i16{1000}), mod01::kI16Min);
    CHECK_EQ(mod01::saturating_mul(i16{0}, mod01::kI16Max), i16{0});
}

TEST_REQ(Saturation, evaluation_a_la_compilation, "LLR-M01-014") {
    // `constexpr` : la valeur est calculee par le compilateur. Zero cycle a
    // l'execution, et une erreur de calcul devient une erreur de COMPILATION.
    constexpr i16 result = mod01::saturating_add(mod01::kI16Max, i16{10});
    static_assert(result == mod01::kI16Max, "la saturation doit etre constexpr");
    CHECK_EQ(result, mod01::kI16Max);
}

// -----------------------------------------------------------------------------
//  Operations verifiees
// -----------------------------------------------------------------------------
TEST_REQ(Checked, addition_nominale, "LLR-M01-020") {
    i32 result = 0;
    CHECK(mod01::checked_add(2000000000, 100, result));
    CHECK_EQ(result, 2000000100);
}

TEST_REQ(Checked, addition_debordement_positif, "LLR-M01-021") {
    constexpr i32 kMax = std::numeric_limits<i32>::max();
    i32 result = 42;
    CHECK_FALSE(mod01::checked_add(kMax, 1, result));
    CHECK_EQ(result, 0);  // sortie neutralisee : pas de valeur trompeuse
}

TEST_REQ(Checked, addition_debordement_negatif, "LLR-M01-022") {
    constexpr i32 kMin = std::numeric_limits<i32>::min();
    i32 result = 42;
    CHECK_FALSE(mod01::checked_add(kMin, -1, result));
    CHECK_EQ(result, 0);
}

TEST_REQ(Checked, division_par_zero, "LLR-M01-023") {
    i32 result = 42;
    CHECK_FALSE(mod01::checked_div(100, 0, result));
    CHECK_EQ(result, 0);
}

TEST_REQ(Checked, division_cas_min_sur_moins_un, "LLR-M01-024") {
    constexpr i32 kMin = std::numeric_limits<i32>::min();
    i32 result = 42;
    CHECK_FALSE(mod01::checked_div(kMin, -1, result));
    CHECK(mod01::checked_div(kMin, 2, result));
    CHECK_EQ(result, kMin / 2);
}

// -----------------------------------------------------------------------------
//  Conversions
// -----------------------------------------------------------------------------
TEST_REQ(Cast, retrecissement_valide, "LLR-M01-030") {
    u8 small = 0U;
    CHECK(mod01::checked_cast<u8>(i32{200}, small));
    CHECK_EQ(small, u8{200U});
}

TEST_REQ(Cast, retrecissement_refuse, "LLR-M01-031") {
    // Le scenario Ariane 5 : une valeur qui ne tient pas dans la cible.
    u8 small = 99U;
    CHECK_FALSE(mod01::checked_cast<u8>(i32{300}, small));
    CHECK_EQ(small, u8{0U});

    i16 short_value = 99;
    CHECK_FALSE(mod01::checked_cast<i16>(i32{40000}, short_value));
    CHECK_EQ(short_value, i16{0});
}

TEST_REQ(Cast, signe_vers_non_signe_negatif_refuse, "LLR-M01-032") {
    // static_cast<u16>(-1) vaut 65535 : silencieux et faux.
    u16 destination = 7U;
    CHECK_FALSE(mod01::checked_cast<u16>(i32{-1}, destination));
    CHECK_EQ(destination, u16{0U});
}

TEST_REQ(Cast, non_signe_vers_signe_trop_grand_refuse, "LLR-M01-033") {
    i16 destination = 7;
    CHECK_FALSE(mod01::checked_cast<i16>(avio::u32{40000U}, destination));
    CHECK(mod01::checked_cast<i16>(avio::u32{30000U}, destination));
    CHECK_EQ(destination, i16{30000});
}

TEST_REQ(Cast, elargissement_toujours_valide, "LLR-M01-034") {
    i32 large = 0;
    CHECK(mod01::checked_cast<i32>(i16{-1234}, large));
    CHECK_EQ(large, -1234);
}

// -----------------------------------------------------------------------------
//  Intervalles
// -----------------------------------------------------------------------------
TEST_REQ(Range, bornes_incluses, "LLR-M01-040") {
    CHECK(mod01::in_range(0, 0, 10));
    CHECK(mod01::in_range(10, 0, 10));
    CHECK_FALSE(mod01::in_range(-1, 0, 10));
    CHECK_FALSE(mod01::in_range(11, 0, 10));
}

TEST_REQ(Range, ecretage, "LLR-M01-041") {
    CHECK_EQ(mod01::clamp(-5, 0, 10), 0);
    CHECK_EQ(mod01::clamp(5, 0, 10), 5);
    CHECK_EQ(mod01::clamp(50, 0, 10), 10);
}

// -----------------------------------------------------------------------------
//  Disposition memoire
// -----------------------------------------------------------------------------
TEST_REQ(Layout, bourrage_observable, "LLR-M01-050") {
    // Ces tailles dependent de l'ABI. Les figer par un test, c'est detecter
    // immediatement un changement de cible ou d'option de compilation.
    CHECK_EQ(sizeof(mod01::NaiveFrame), static_cast<avio::usize>(12));
    CHECK_EQ(sizeof(mod01::CompactFrame), static_cast<avio::usize>(8));
    CHECK(sizeof(mod01::CompactFrame) < sizeof(mod01::NaiveFrame));
}
