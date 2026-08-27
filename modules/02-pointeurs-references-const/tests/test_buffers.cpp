#include <avio/span.hpp>
#include <avio/types.hpp>
#include <microtest/microtest.hpp>

#include "mod02/buffers.hpp"

using avio::i32;
using avio::u16;
using avio::u8;
using avio::usize;

// -----------------------------------------------------------------------------
//  References et pointeurs
// -----------------------------------------------------------------------------
TEST_REQ(Reference, echange_de_valeurs, "LLR-M02-001") {
    i32 a = 1;
    i32 b = 2;
    mod02::swap_values(a, b);
    CHECK_EQ(a, 2);
    CHECK_EQ(b, 1);
}

TEST_REQ(Pointeur, increment_valide, "LLR-M02-002") {
    i32 value = 41;
    CHECK(mod02::increment_if_valid(&value));
    CHECK_EQ(value, 42);
}

TEST_REQ(Pointeur, robustesse_pointeur_nul, "LLR-M02-003") {
    // Cas de robustesse : la branche `value == nullptr` DOIT etre couverte,
    // sinon elle apparaitra comme du code non atteint lors de l'analyse de
    // couverture structurelle (module 11).
    CHECK_FALSE(mod02::increment_if_valid(nullptr));
}

// -----------------------------------------------------------------------------
//  find_max
// -----------------------------------------------------------------------------
TEST_REQ(FindMax, cas_nominal, "LLR-M02-010") {
    i32 values[5] = {3, 17, -4, 17, 0};
    i32 maximum = 0;
    REQUIRE(mod02::find_max(avio::make_const_span(values), maximum));
    CHECK_EQ(maximum, 17);
}

TEST_REQ(FindMax, un_seul_element, "LLR-M02-011") {
    i32 values[1] = {-9};
    i32 maximum = 0;
    REQUIRE(mod02::find_max(avio::make_const_span(values), maximum));
    CHECK_EQ(maximum, -9);
}

TEST_REQ(FindMax, toutes_valeurs_negatives, "LLR-M02-012") {
    // Test qui echouerait si l'implementation initialisait le maximum a 0
    // au lieu du premier element. Erreur classique.
    i32 values[3] = {-5, -2, -100};
    i32 maximum = 999;
    REQUIRE(mod02::find_max(avio::make_const_span(values), maximum));
    CHECK_EQ(maximum, -2);
}

TEST_REQ(FindMax, robustesse_tampon_vide, "LLR-M02-013") {
    const avio::Span<const i32> empty;
    i32 maximum = 1234;
    CHECK_FALSE(mod02::find_max(empty, maximum));
    // La sortie ne doit PAS avoir ete touchee : l'appelant peut se fier a son
    // etat anterieur.
    CHECK_EQ(maximum, 1234);
}

// -----------------------------------------------------------------------------
//  checksum16
// -----------------------------------------------------------------------------
TEST_REQ(Checksum, deterministe, "LLR-M02-020") {
    u8 frame[4] = {0x12U, 0x34U, 0x56U, 0x78U};
    const u16 first = mod02::checksum16(avio::make_const_span(frame));
    const u16 second = mod02::checksum16(avio::make_const_span(frame));
    CHECK_EQ(first, second);
}

TEST_REQ(Checksum, detecte_une_modification, "LLR-M02-021") {
    u8 frame[4] = {0x12U, 0x34U, 0x56U, 0x78U};
    const u16 before = mod02::checksum16(avio::make_const_span(frame));
    frame[2] = 0x57U;
    const u16 after = mod02::checksum16(avio::make_const_span(frame));
    CHECK(before != after);
}

TEST_REQ(Checksum, longueur_impaire, "LLR-M02-022") {
    u8 frame[3] = {0xAAU, 0xBBU, 0xCCU};
    const u16 result = mod02::checksum16(avio::make_const_span(frame));
    // Valeur de reference calculee a la main :
    //   0xAABB + 0xCC00 = 0x176BB -> repliement -> 0x76BB + 1 = 0x76BC
    //   complement -> 0x8943
    CHECK_EQ(result, u16{0x8943U});
}

TEST_REQ(Checksum, robustesse_tampon_vide, "LLR-M02-023") {
    const avio::Span<const u8> empty;
    CHECK_EQ(mod02::checksum16(empty), u16{0xFFFFU});
}

// -----------------------------------------------------------------------------
//  copy_bounded
// -----------------------------------------------------------------------------
TEST_REQ(Copy, destination_plus_grande, "LLR-M02-030") {
    u8 source[3] = {1U, 2U, 3U};
    u8 destination[5] = {9U, 9U, 9U, 9U, 9U};

    const usize copies =
        mod02::copy_bounded(avio::make_const_span(source), avio::make_span(destination));
    CHECK_EQ(copies, usize{3});
    CHECK_EQ(destination[0], u8{1U});
    CHECK_EQ(destination[2], u8{3U});
    // Au-dela, la destination est intacte.
    CHECK_EQ(destination[3], u8{9U});
}

TEST_REQ(Copy, destination_plus_petite_pas_de_debordement, "LLR-M02-031") {
    // LE test qui compte : c'est ce scenario qui produit les debordements de
    // tampon (CWE-787) dans le monde reel.
    u8 source[5] = {1U, 2U, 3U, 4U, 5U};
    u8 destination[3] = {0U, 0U, 0U};
    u8 sentinel = 0xEEU;

    const usize copies =
        mod02::copy_bounded(avio::make_const_span(source), avio::make_span(destination));
    CHECK_EQ(copies, usize{3});
    CHECK_EQ(destination[2], u8{3U});
    CHECK_EQ(sentinel, u8{0xEEU});  // rien n'a deborde
}

TEST_REQ(Copy, robustesse_source_vide, "LLR-M02-032") {
    u8 destination[2] = {7U, 8U};
    const avio::Span<const u8> empty;
    CHECK_EQ(mod02::copy_bounded(empty, avio::make_span(destination)), usize{0});
    CHECK_EQ(destination[0], u8{7U});
}

// -----------------------------------------------------------------------------
//  fill / equals
// -----------------------------------------------------------------------------
TEST_REQ(Fill, remplissage_complet, "LLR-M02-040") {
    u8 buffer[4] = {};
    mod02::fill(avio::make_span(buffer), 0x5AU);
    for (usize i = 0U; i < 4U; ++i) {
        CHECK_EQ(buffer[i], u8{0x5AU});
    }
}

TEST_REQ(Equals, egalite_et_difference, "LLR-M02-041") {
    u8 a[3] = {1U, 2U, 3U};
    u8 b[3] = {1U, 2U, 3U};
    u8 c[3] = {1U, 2U, 4U};
    u8 d[2] = {1U, 2U};

    CHECK(mod02::equals(avio::make_const_span(a), avio::make_const_span(b)));
    CHECK_FALSE(mod02::equals(avio::make_const_span(a), avio::make_const_span(c)));
    CHECK_FALSE(mod02::equals(avio::make_const_span(a), avio::make_const_span(d)));
}

// -----------------------------------------------------------------------------
//  MeasurementLog : const-correctness et comportement circulaire
// -----------------------------------------------------------------------------
TEST_REQ(Log, etat_initial, "LLR-M02-050") {
    const mod02::MeasurementLog log;
    CHECK_EQ(log.size(), usize{0});
    CHECK_FALSE(log.has_overflowed());

    i32 value = 0;
    CHECK_FALSE(log.at(0U, value));
}

TEST_REQ(Log, remplissage_partiel, "LLR-M02-051") {
    mod02::MeasurementLog log;
    log.push(10);
    log.push(20);
    log.push(30);

    CHECK_EQ(log.size(), usize{3});
    CHECK_FALSE(log.has_overflowed());

    i32 value = 0;
    REQUIRE(log.at(0U, value));
    CHECK_EQ(value, 10);
    REQUIRE(log.at(2U, value));
    CHECK_EQ(value, 30);
}

TEST_REQ(Log, capacite_exacte, "LLR-M02-052") {
    mod02::MeasurementLog log;
    for (i32 i = 0; i < 8; ++i) {
        log.push(i);
    }
    CHECK_EQ(log.size(), mod02::MeasurementLog::kCapacity);
    CHECK_FALSE(log.has_overflowed());  // exactement plein, pas encore ecrase

    i32 value = 0;
    REQUIRE(log.at(0U, value));
    CHECK_EQ(value, 0);
}

TEST_REQ(Log, ecrasement_circulaire, "LLR-M02-053") {
    mod02::MeasurementLog log;
    for (i32 i = 0; i < 10; ++i) {
        log.push(i);
    }
    CHECK_EQ(log.size(), mod02::MeasurementLog::kCapacity);
    CHECK(log.has_overflowed());

    // Les deux plus anciennes (0 et 1) ont ete ecrasees : le journal contient
    // maintenant 2..9, du plus ancien au plus recent.
    i32 value = 0;
    REQUIRE(log.at(0U, value));
    CHECK_EQ(value, 2);
    REQUIRE(log.at(7U, value));
    CHECK_EQ(value, 9);
}

TEST_REQ(Log, robustesse_index_hors_domaine, "LLR-M02-054") {
    mod02::MeasurementLog log;
    log.push(1);

    i32 value = 777;
    CHECK_FALSE(log.at(1U, value));
    CHECK_FALSE(log.at(1000U, value));
    CHECK_EQ(value, 777);  // sortie non modifiee
}

TEST_REQ(Log, remise_a_zero, "LLR-M02-055") {
    mod02::MeasurementLog log;
    for (i32 i = 0; i < 12; ++i) {
        log.push(i);
    }
    log.clear();
    CHECK_EQ(log.size(), usize{0});
    CHECK_FALSE(log.has_overflowed());
}

TEST_REQ(Log, vue_lecture_seule, "LLR-M02-056") {
    mod02::MeasurementLog log;
    log.push(5);

    // `journal_const` ne donne acces qu'aux methodes const : c'est le
    // compilateur qui garantit l'absence de modification, pas une convention.
    const mod02::MeasurementLog& const_log = log;
    const avio::Span<const i32> view = const_log.raw_storage();
    CHECK_EQ(view.size(), mod02::MeasurementLog::kCapacity);
    CHECK_EQ(view[0], 5);
    // vue[0] = 42;  // <-- ne compile pas : Span<const i32>
}
