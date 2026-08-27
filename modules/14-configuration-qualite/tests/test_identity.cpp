#include <avio/span.hpp>
#include <avio/types.hpp>
#include <microtest/microtest.hpp>

#include "mod14/identity.hpp"

using avio::u32;
using avio::u8;
using avio::usize;
using mod14::ControlCategory;
using mod14::SoftwareIdentity;

namespace {

/// "123456789" : le vecteur de test standard de tous les CRC.
constexpr u8 kVecteurStandard[9] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U};

}  // namespace

// =============================================================================
//  CRC-32
// =============================================================================
TEST_REQ(Crc32, vecteur_de_reference, "LLR-CM-010") {
    CHECK_EQ(mod14::crc32(avio::make_const_span(kVecteurStandard)), u32{0xCBF43926U});
}

TEST_REQ(Crc32, tampon_vide, "LLR-CM-010") {
    const avio::Span<const u8> vide;
    CHECK_EQ(mod14::crc32(vide), u32{0x00000000U});
}

TEST_REQ(Crc32, detecte_toute_alteration_d_un_bit, "LLR-CM-010") {
    u8 image[9] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U};
    const u32 reference = mod14::crc32(avio::make_const_span(image));

    // Chacun des 72 bits est bascule, un a un : le CRC doit changer a chaque
    // fois. C'est la propriete qui fait de lui un controle d'integrite.
    for (usize octet = 0U; octet < 9U; ++octet) {
        for (u32 bit = 0U; bit < 8U; ++bit) {
            image[octet] = static_cast<u8>(image[octet] ^ static_cast<u8>(1U << bit));
            CHECK(mod14::crc32(avio::make_const_span(image)) != reference);
            image[octet] = static_cast<u8>(image[octet] ^ static_cast<u8>(1U << bit));
        }
    }
    CHECK_EQ(mod14::crc32(avio::make_const_span(image)), reference);
}

TEST_REQ(Crc32, table_en_memoire_morte, "LLR-CM-010") {
    // Deja verifie par static_assert a la compilation ; le retester ici
    // demontre que la table EMBARQUEE est bien celle qui a ete calculee.
    CHECK_EQ(mod14::kCrc32Table.values[0], u32{0x00000000U});
    CHECK_EQ(mod14::kCrc32Table.values[1], u32{0x77073096U});
    CHECK_EQ(mod14::kCrc32Table.values[255], u32{0x2D02EF8DU});
}

// =============================================================================
//  Part number
// =============================================================================
TEST_REQ(PartNumber, format_valide, "LLR-CM-020") {
    CHECK(mod14::is_valid_part_number("PN-1234567-001"));
    CHECK(mod14::is_valid_part_number("PN-0000000-000"));
    CHECK(mod14::is_valid_part_number("PN-9999999-999"));
}

TEST_REQ(PartNumber, format_invalide, "LLR-CM-020") {
    CHECK_FALSE(mod14::is_valid_part_number(nullptr));
    CHECK_FALSE(mod14::is_valid_part_number(""));
    CHECK_FALSE(mod14::is_valid_part_number("PN-1234567-01"));    // trop court
    CHECK_FALSE(mod14::is_valid_part_number("PN-1234567-0011"));  // trop long
    CHECK_FALSE(mod14::is_valid_part_number("XX-1234567-001"));   // prefixe
    CHECK_FALSE(mod14::is_valid_part_number("PN_1234567-001"));   // separateur
    CHECK_FALSE(mod14::is_valid_part_number("PN-123456A-001"));   // non chiffre
    CHECK_FALSE(mod14::is_valid_part_number("PN-1234567_001"));   // separateur
    CHECK_FALSE(mod14::is_valid_part_number("PN-1234567-0A1"));   // non chiffre
}

// =============================================================================
//  Verification du chargement
// =============================================================================
TEST_REQ(Chargement, image_conforme, "LLR-CM-030") {
    SoftwareIdentity identite;
    identite.part_number = "PN-7654321-002";
    identite.version_major = 1U;
    identite.version_minor = 2U;
    identite.version_patch = 3U;
    identite.expected_crc = 0xCBF43926U;

    CHECK(mod14::verify_load(identite, avio::make_const_span(kVecteurStandard)));
}

TEST_REQ(Chargement, image_alteree_refusee, "LLR-CM-030") {
    SoftwareIdentity identite;
    identite.part_number = "PN-7654321-002";
    identite.expected_crc = 0xCBF43926U;

    u8 alteree[9] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x38U};
    CHECK_FALSE(mod14::verify_load(identite, avio::make_const_span(alteree)));
}

TEST_REQ(Chargement, part_number_invalide_refuse, "LLR-CM-030") {
    SoftwareIdentity identite;
    identite.part_number = "MAUVAIS";
    identite.expected_crc = 0xCBF43926U;
    CHECK_FALSE(mod14::verify_load(identite, avio::make_const_span(kVecteurStandard)));
}

TEST_REQ(Chargement, image_vide_refusee, "LLR-CM-030") {
    SoftwareIdentity identite;
    identite.part_number = "PN-7654321-002";
    identite.expected_crc = 0x00000000U;  // CRC d'un tampon vide

    const avio::Span<const u8> vide;
    // Meme si le CRC correspondait, une image vide n'est PAS un chargement
    // valide. La verification de presence est SPECIFIEE avant celle du CRC.
    CHECK_FALSE(mod14::verify_load(identite, vide));
}

// =============================================================================
//  Donnees de vie du logiciel et categories de controle
// =============================================================================
TEST_REQ(DonneesDeVie, table_complete, "LLR-CM-040") {
    const avio::Span<const mod14::LifeCycleData> table = mod14::life_cycle_data();
    CHECK_EQ(table.size(), usize{20});

    // Chaque entree porte un acronyme et un intitule non vides.
    for (usize index = 0U; index < table.size(); ++index) {
        CHECK(table[index].acronym != nullptr);
        CHECK(table[index].name != nullptr);
        CHECK(table[index].acronym[0] != '\0');
        CHECK(table[index].section >= 1U);
        CHECK(table[index].section <= 20U);
    }
}

TEST_REQ(DonneesDeVie, recherche_par_acronyme, "LLR-CM-041") {
    const mod14::LifeCycleData* psac = mod14::find_life_cycle_data("PSAC");
    REQUIRE(psac != nullptr);
    CHECK_EQ(psac->acronym, "PSAC");
    CHECK_EQ(psac->dal_ab, ControlCategory::CC1);
    CHECK_EQ(psac->dal_cd, ControlCategory::CC1);

    CHECK(mod14::find_life_cycle_data("INEXISTANT") == nullptr);
    CHECK(mod14::find_life_cycle_data(nullptr) == nullptr);
}

TEST_REQ(DonneesDeVie, categorie_selon_le_niveau, "LLR-CM-042") {
    ControlCategory categorie = ControlCategory::CC2;

    // Le SDD est CC1 en DAL A/B, mais CC2 en DAL C/D : la rigueur exigee
    // depend du NIVEAU, pas de l'importance intrinseque du document.
    REQUIRE(mod14::control_category_for("SDD", 'A', categorie));
    CHECK_EQ(categorie, ControlCategory::CC1);
    REQUIRE(mod14::control_category_for("SDD", 'B', categorie));
    CHECK_EQ(categorie, ControlCategory::CC1);
    REQUIRE(mod14::control_category_for("SDD", 'C', categorie));
    CHECK_EQ(categorie, ControlCategory::CC2);
    REQUIRE(mod14::control_category_for("SDD", 'D', categorie));
    CHECK_EQ(categorie, ControlCategory::CC2);
}

TEST_REQ(DonneesDeVie, donnees_toujours_cc1, "LLR-CM-042") {
    // Certaines donnees restent CC1 quel que soit le niveau : le code source,
    // l'executable, les exigences, et les index de configuration. Ce sont
    // celles sans lesquelles on ne peut pas reconstruire ni identifier le
    // produit.
    const char* toujours_cc1[6] = {"PSAC", "SRD", "SRC", "EOC", "SECI", "SCI"};
    ControlCategory categorie = ControlCategory::CC2;

    for (usize index = 0U; index < 6U; ++index) {
        REQUIRE(mod14::control_category_for(toujours_cc1[index], 'A', categorie));
        CHECK_EQ(categorie, ControlCategory::CC1);
        REQUIRE(mod14::control_category_for(toujours_cc1[index], 'D', categorie));
        CHECK_EQ(categorie, ControlCategory::CC1);
    }
}

TEST_REQ(DonneesDeVie, robustesse_niveau_invalide, "LLR-CM-042") {
    ControlCategory categorie = ControlCategory::CC2;
    // DAL E : aucun objectif DO-178C, donc aucune categorie de controle.
    CHECK_FALSE(mod14::control_category_for("SDD", 'E', categorie));
    CHECK_FALSE(mod14::control_category_for("SDD", 'Z', categorie));
    CHECK_FALSE(mod14::control_category_for("INEXISTANT", 'A', categorie));
}

TEST_REQ(DonneesDeVie, libelles_de_categorie, "LLR-CM-042") {
    CHECK_EQ(mod14::category_name(ControlCategory::CC1), "CC1");
    CHECK_EQ(mod14::category_name(ControlCategory::CC2), "CC2");
    CHECK_EQ(mod14::category_name(static_cast<ControlCategory>(u8{9U})), "inconnue");
}
