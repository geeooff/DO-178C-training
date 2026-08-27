// =============================================================================
//  Module 13 -- LES MEMES cas de test appliques aux deux implementations.
//
//  Elles passent toutes les deux. C'est LA lecon du module : un standard de
//  codage ne parle pas de correction fonctionnelle. Il parle de
//  VERIFIABILITE, de LISIBILITE et de PREVISIBILITE -- donc du COUT de la
//  verification, de la maintenance et de la certification.
// =============================================================================
#include <avio/types.hpp>
#include <microtest/microtest.hpp>

#include "mod13/bcd.hpp"

using avio::u16;
using avio::u32;
using avio::usize;
using mod07::Status;

namespace {

struct BcdCase {
    u16 word;
    u32 expected;
    bool valid;
};

/// Jeu de test commun aux deux implementations.
constexpr BcdCase kCase[9] = {{0x0000U, 0U, true},    {0x0001U, 1U, true},  {0x1234U, 1234U, true},
                              {0x9999U, 9999U, true}, {0x0090U, 90U, true}, {0x000AU, 0U, false},
                              {0x00A0U, 0U, false},   {0xA000U, 0U, false}, {0x12F4U, 0U, false}};

}  // namespace

// =============================================================================
//  Version CONFORME
// =============================================================================
TEST_REQ(Conforme, conversion_bcd, "LLR-BCD-010") {
    for (usize index = 0U; index < 9U; ++index) {
        const mod07::Result<u32> result = mod13::bcd_to_binary(kCase[index].word);
        if (kCase[index].valid) {
            REQUIRE(result.is_ok());
            CHECK_EQ(result.value(), kCase[index].expected);
        } else {
            CHECK_EQ(result.status(), Status::InvalidArgument);
        }
    }
}

TEST_REQ(Conforme, groupe_invalide_detecte, "LLR-BCD-011") {
    // Les six valeurs invalides d'un groupe BCD : 10 a 15.
    for (u32 digit = 10U; digit <= 15U; ++digit) {
        const u16 word = static_cast<u16>(digit);
        CHECK_EQ(mod13::bcd_to_binary(word).status(), Status::InvalidArgument);
    }
    // Et les dix valides.
    for (u32 digit = 0U; digit <= 9U; ++digit) {
        const u16 word = static_cast<u16>(digit);
        const mod07::Result<u32> result = mod13::bcd_to_binary(word);
        REQUIRE(result.is_ok());
        CHECK_EQ(result.value(), digit);
    }
}

TEST_REQ(Conforme, conversion_inverse, "LLR-BCD-020") {
    const mod07::Result<u16> thousand_two_hundred_thirty_four = mod13::binary_to_bcd(1234U);
    REQUIRE(thousand_two_hundred_thirty_four.is_ok());
    CHECK_EQ(thousand_two_hundred_thirty_four.value(), u16{0x1234U});

    const mod07::Result<u16> zero = mod13::binary_to_bcd(0U);
    REQUIRE(zero.is_ok());
    CHECK_EQ(zero.value(), u16{0x0000U});

    const mod07::Result<u16> maximum = mod13::binary_to_bcd(9999U);
    REQUIRE(maximum.is_ok());
    CHECK_EQ(maximum.value(), u16{0x9999U});
}

TEST_REQ(Conforme, conversion_inverse_hors_domaine, "LLR-BCD-020") {
    CHECK_EQ(mod13::binary_to_bcd(10000U).status(), Status::OutOfRange);
    CHECK_EQ(mod13::binary_to_bcd(4294967295U).status(), Status::OutOfRange);
}

TEST_REQ(Conforme, aller_retour, "LLR-BCD-010,LLR-BCD-020") {
    // Propriete : pour toute valeur du domaine, bcd(bin(v)) == v.
    for (u32 value = 0U; value <= 9999U; value += 37U) {
        const mod07::Result<u16> encode = mod13::binary_to_bcd(value);
        REQUIRE(encode.is_ok());
        const mod07::Result<u32> decode = mod13::bcd_to_binary(encode.value());
        REQUIRE(decode.is_ok());
        CHECK_EQ(decode.value(), value);
    }
}

// =============================================================================
//  Version NON CONFORME : MEMES cas, MEME resultat fonctionnel
// =============================================================================
TEST_REQ(NonConforme, memes_resultats_fonctionnels, "LLR-BCD-030") {
    for (usize index = 0U; index < 9U; ++index) {
        const u32 actual = mod13_nonconforming::bcd_to_binary(kCase[index].word);
        if (kCase[index].valid) {
            CHECK_EQ(actual, kCase[index].expected);
        } else {
            // Convention d'erreur DIFFERENTE : une valeur sentinelle, du meme
            // type que le resultat valide. Rien dans le TYPE ne distingue une
            // erreur d'un succes : l'appelant PEUT l'ignorer, et il le fera.
            CHECK_EQ(actual, u32{0xFFFFFFFFU});
        }
    }
}

TEST_REQ(NonConforme, equivalence_avec_la_version_conforme, "LLR-BCD-030") {
    // Les deux implementations sont equivalentes sur tout le domaine valide.
    // Autrement dit : les tests FONCTIONNELS ne feront JAMAIS la difference.
    // Seuls la revue de code et l'analyse statique la font.
    for (u32 value = 0U; value <= 9999U; value += 13U) {
        const mod07::Result<u16> encode = mod13::binary_to_bcd(value);
        REQUIRE(encode.is_ok());

        const mod07::Result<u32> conforming = mod13::bcd_to_binary(encode.value());
        const u32 nonconforming = mod13_nonconforming::bcd_to_binary(encode.value());

        REQUIRE(conforming.is_ok());
        CHECK_EQ(conforming.value(), nonconforming);
    }
}

TEST_REQ(NonConforme, la_sentinelle_est_ambigue, "LLR-BCD-030") {
    // Demonstration du danger de la valeur sentinelle : elle occupe une place
    // dans le domaine du type de retour. Ici 0xFFFFFFFF n'est pas atteignable
    // par une conversion valide (le maximum est 9999), mais c'est un COUP DE
    // CHANCE lie au domaine restreint. Sur un type plus etroit -- disons u16
    // avec un maximum de 65535 -- il n'y aurait plus de valeur libre.
    //
    // Result<T> n'a pas ce probleme : le statut est un CHAMP SEPARE.
    const mod07::Result<u32> conforming = mod13::bcd_to_binary(0x000AU);
    CHECK(conforming.is_error());
    CHECK_EQ(conforming.value_or(0U), u32{0U});

    const u32 nonconforming = mod13_nonconforming::bcd_to_binary(0x000AU);
    CHECK_EQ(nonconforming, u32{0xFFFFFFFFU});
    // Rien, dans le type `u32`, ne dit que cette valeur est une erreur.
}
