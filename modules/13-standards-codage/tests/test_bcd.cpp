// =============================================================================
//  Module 13 -- LES MEMES cas de test appliques aux deux implementations.
//
//  Elles passent toutes les deux. C'est LA lecon du module : un standard de
//  codage ne parle pas de correction fonctionnelle. Il parle de
//  VERIFIABILITE, de LISIBILITE et de PREVISIBILITE -- donc du COUT de la
//  verification, de la maintenance et de la certification.
// =============================================================================
#include <microtest/microtest.hpp>

#include "mod13/bcd.hpp"

#include <avio/types.hpp>

using avio::u16;
using avio::u32;
using avio::usize;
using mod07::Status;

namespace {

struct CasBcd {
    u16 mot;
    u32 attendu;
    bool valide;
};

/// Jeu de test commun aux deux implementations.
constexpr CasBcd kCas[9] = {
    {0x0000U, 0U, true},      {0x0001U, 1U, true},    {0x1234U, 1234U, true},
    {0x9999U, 9999U, true},   {0x0090U, 90U, true},   {0x000AU, 0U, false},
    {0x00A0U, 0U, false},     {0xA000U, 0U, false},   {0x12F4U, 0U, false}};

}  // namespace

// =============================================================================
//  Version CONFORME
// =============================================================================
TEST_REQ(Conforme, conversion_bcd, "LLR-BCD-010") {
    for (usize index = 0U; index < 9U; ++index) {
        const mod07::Result<u32> resultat = mod13::bcd_to_binary(kCas[index].mot);
        if (kCas[index].valide) {
            REQUIRE(resultat.is_ok());
            CHECK_EQ(resultat.value(), kCas[index].attendu);
        } else {
            CHECK_EQ(resultat.status(), Status::InvalidArgument);
        }
    }
}

TEST_REQ(Conforme, groupe_invalide_detecte, "LLR-BCD-011") {
    // Les six valeurs invalides d'un groupe BCD : 10 a 15.
    for (u32 chiffre = 10U; chiffre <= 15U; ++chiffre) {
        const u16 mot = static_cast<u16>(chiffre);
        CHECK_EQ(mod13::bcd_to_binary(mot).status(), Status::InvalidArgument);
    }
    // Et les dix valides.
    for (u32 chiffre = 0U; chiffre <= 9U; ++chiffre) {
        const u16 mot = static_cast<u16>(chiffre);
        const mod07::Result<u32> resultat = mod13::bcd_to_binary(mot);
        REQUIRE(resultat.is_ok());
        CHECK_EQ(resultat.value(), chiffre);
    }
}

TEST_REQ(Conforme, conversion_inverse, "LLR-BCD-020") {
    const mod07::Result<u16> mille_deux_cent_trente_quatre = mod13::binary_to_bcd(1234U);
    REQUIRE(mille_deux_cent_trente_quatre.is_ok());
    CHECK_EQ(mille_deux_cent_trente_quatre.value(), u16{0x1234U});

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
    for (u32 valeur = 0U; valeur <= 9999U; valeur += 37U) {
        const mod07::Result<u16> encode = mod13::binary_to_bcd(valeur);
        REQUIRE(encode.is_ok());
        const mod07::Result<u32> decode = mod13::bcd_to_binary(encode.value());
        REQUIRE(decode.is_ok());
        CHECK_EQ(decode.value(), valeur);
    }
}

// =============================================================================
//  Version NON CONFORME : MEMES cas, MEME resultat fonctionnel
// =============================================================================
TEST_REQ(NonConforme, memes_resultats_fonctionnels, "LLR-BCD-030") {
    for (usize index = 0U; index < 9U; ++index) {
        const u32 obtenu = mod13_nonconforming::bcd_to_binary(kCas[index].mot);
        if (kCas[index].valide) {
            CHECK_EQ(obtenu, kCas[index].attendu);
        } else {
            // Convention d'erreur DIFFERENTE : une valeur sentinelle, du meme
            // type que le resultat valide. Rien dans le TYPE ne distingue une
            // erreur d'un succes : l'appelant PEUT l'ignorer, et il le fera.
            CHECK_EQ(obtenu, u32{0xFFFFFFFFU});
        }
    }
}

TEST_REQ(NonConforme, equivalence_avec_la_version_conforme, "LLR-BCD-030") {
    // Les deux implementations sont equivalentes sur tout le domaine valide.
    // Autrement dit : les tests FONCTIONNELS ne feront JAMAIS la difference.
    // Seuls la revue de code et l'analyse statique la font.
    for (u32 valeur = 0U; valeur <= 9999U; valeur += 13U) {
        const mod07::Result<u16> encode = mod13::binary_to_bcd(valeur);
        REQUIRE(encode.is_ok());

        const mod07::Result<u32> conforme = mod13::bcd_to_binary(encode.value());
        const u32 non_conforme = mod13_nonconforming::bcd_to_binary(encode.value());

        REQUIRE(conforme.is_ok());
        CHECK_EQ(conforme.value(), non_conforme);
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
    const mod07::Result<u32> conforme = mod13::bcd_to_binary(0x000AU);
    CHECK(conforme.is_error());
    CHECK_EQ(conforme.value_or(0U), u32{0U});

    const u32 non_conforme = mod13_nonconforming::bcd_to_binary(0x000AU);
    CHECK_EQ(non_conforme, u32{0xFFFFFFFFU});
    // Rien, dans le type `u32`, ne dit que cette valeur est une erreur.
}
