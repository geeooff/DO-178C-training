#include <avio/span.hpp>
#include <avio/types.hpp>
#include <microtest/microtest.hpp>

#include "mod06/compile_time.hpp"
#include "mod06/ring_buffer.hpp"
#include "mod06/static_polymorphism.hpp"

using avio::f32;
using avio::i32;
using avio::u32;
using avio::u8;
using avio::usize;

// =============================================================================
//  1. RingBuffer -- COUVERTURE PAR INSTANCIATION
//
//  Chaque instanciation embarquee est testee SEPAREMENT. C'est l'exigence
//  DO-332 sur le polymorphisme parametrique : la couverture porte sur le code
//  executable, et chaque instanciation en produit un different.
// =============================================================================

TEST_REQ(RingBuffer_i32_4, cycle_de_vie_complet, "LLR-M06-001") {
    mod06::RingBuffer<i32, 4U> tampon;

    CHECK(tampon.empty());
    CHECK_FALSE(tampon.full());
    CHECK_EQ(tampon.size(), usize{0});
    CHECK_EQ(tampon.kCapacity, usize{4});

    CHECK(tampon.push(10));
    CHECK(tampon.push(20));
    CHECK(tampon.push(30));
    CHECK(tampon.push(40));
    CHECK(tampon.full());
    CHECK_EQ(tampon.overwrite_count(), u32{0});

    // Cinquieme element : le plus ancien (10) est ecrase.
    CHECK_FALSE(tampon.push(50));
    CHECK_EQ(tampon.overwrite_count(), u32{1});
    CHECK_EQ(tampon.size(), usize{4});

    i32 valeur = 0;
    REQUIRE(tampon.pop(valeur));
    CHECK_EQ(valeur, 20);  // 10 a disparu
    REQUIRE(tampon.peek(0U, valeur));
    CHECK_EQ(valeur, 30);
}

TEST_REQ(RingBuffer_i32_4, robustesse_tampon_vide, "LLR-M06-002") {
    mod06::RingBuffer<i32, 4U> tampon;
    i32 valeur = 999;
    CHECK_FALSE(tampon.pop(valeur));
    CHECK_FALSE(tampon.peek(0U, valeur));
    CHECK_EQ(valeur, 999);  // sortie non modifiee
}

TEST_REQ(RingBuffer_i32_4, remise_a_zero, "LLR-M06-003") {
    mod06::RingBuffer<i32, 4U> tampon;
    for (i32 index = 0; index < 10; ++index) {
        (void)tampon.push(index);
    }
    CHECK(tampon.overwrite_count() > 0U);
    tampon.clear();
    CHECK(tampon.empty());
    CHECK_EQ(tampon.overwrite_count(), u32{0});
}

TEST_REQ(RingBuffer_f32_8, instanciation_flottante, "LLR-M06-004") {
    // MEME code source, AUTRE code executable : il faut le couvrir aussi.
    mod06::RingBuffer<f32, 8U> tampon;
    CHECK_EQ(tampon.kCapacity, usize{8});

    for (u32 index = 0U; index < 8U; ++index) {
        CHECK(tampon.push(static_cast<f32>(index) * 1.5F));
    }
    CHECK(tampon.full());

    f32 valeur = 0.0F;
    REQUIRE(tampon.peek(2U, valeur));
    CHECK_NEAR(static_cast<double>(valeur), 3.0, 1e-6);
}

TEST_REQ(RingBuffer_u8_16, instanciation_octet, "LLR-M06-005") {
    mod06::RingBuffer<u8, 16U> tampon;
    CHECK_EQ(tampon.kCapacity, usize{16});
    for (u32 index = 0U; index < 20U; ++index) {
        (void)tampon.push(static_cast<u8>(index));
    }
    CHECK_EQ(tampon.overwrite_count(), u32{4});
    u8 valeur = 0U;
    REQUIRE(tampon.peek(0U, valeur));
    CHECK_EQ(valeur, u8{4U});  // 0..3 ont ete ecrases
}

TEST_REQ(RingBuffer, taille_memoire_par_instanciation, "LLR-M06-006") {
    // La taille depend des parametres : c'est une donnee d'architecture a
    // documenter (budget RAM par instanciation).
    CHECK(sizeof(mod06::RingBuffer<i32, 4U>) >= (4U * sizeof(i32)));
    CHECK(sizeof(mod06::RingBuffer<u8, 16U>) >= 16U);
    CHECK(sizeof(mod06::RingBuffer<i32, 4U>) != sizeof(mod06::RingBuffer<f32, 8U>));
}

// -----------------------------------------------------------------------------
//  average : les deux branches de `if constexpr` sont DEUX codes distincts
// -----------------------------------------------------------------------------
TEST_REQ(Average, branche_entiere, "LLR-M06-010") {
    mod06::RingBuffer<i32, 4U> tampon;
    (void)tampon.push(10);
    (void)tampon.push(20);
    (void)tampon.push(31);
    // Moyenne entiere : troncature vers zero, 61/3 = 20
    CHECK_EQ(mod06::average(tampon), 20);
}

TEST_REQ(Average, branche_flottante, "LLR-M06-011") {
    mod06::RingBuffer<f32, 8U> tampon;
    (void)tampon.push(1.0F);
    (void)tampon.push(2.0F);
    (void)tampon.push(4.0F);
    CHECK_NEAR(static_cast<double>(mod06::average(tampon)), 2.3333333, 1e-5);
}

TEST_REQ(Average, robustesse_tampon_vide, "LLR-M06-012") {
    const mod06::RingBuffer<i32, 4U> entier;
    const mod06::RingBuffer<f32, 8U> flottant;
    CHECK_EQ(mod06::average(entier), 0);
    CHECK_NEAR(static_cast<double>(mod06::average(flottant)), 0.0, 1e-9);
}

// =============================================================================
//  2. constexpr
// =============================================================================
TEST_REQ(Constexpr, table_crc_en_memoire_morte, "LLR-M06-020") {
    // Ces valeurs ont deja ete verifiees par static_assert A LA COMPILATION.
    // Les retester a l'execution montre que la table embarquee est bien celle
    // que le compilateur a calculee.
    CHECK_EQ(mod06::kCrc8Table.values[0], u8{0x00U});
    CHECK_EQ(mod06::kCrc8Table.values[1], u8{0x07U});
    CHECK_EQ(mod06::kCrc8Table.values[255], u8{0xF3U});
}

TEST_REQ(Constexpr, crc8_valeur_de_reference, "LLR-M06-021") {
    // Vecteur de test standard du CRC-8/SMBUS : la chaine "123456789".
    const u8 message[9] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U};
    CHECK_EQ(mod06::crc8(avio::make_const_span(message)), u8{0xF4U});
}

TEST_REQ(Constexpr, crc8_detecte_une_alteration, "LLR-M06-022") {
    u8 trame[4] = {0x12U, 0x34U, 0x56U, 0x78U};
    const u8 reference = mod06::crc8(avio::make_const_span(trame));
    CHECK_EQ(reference, u8{0x1CU});

    trame[1] = 0x35U;  // un seul bit change
    CHECK(mod06::crc8(avio::make_const_span(trame)) != reference);
}

TEST_REQ(Constexpr, crc8_tampon_vide, "LLR-M06-023") {
    const avio::Span<const u8> vide;
    CHECK_EQ(mod06::crc8(vide, 0x5AU), u8{0x5AU});
}

TEST_REQ(Constexpr, fonctions_utilisables_aux_deux_moments, "LLR-M06-024") {
    // A la compilation :
    constexpr avio::u64 mille_vingt_quatre = mod06::ipow(2U, 10U);
    static_assert(mille_vingt_quatre == 1024U, "evaluation a la compilation attendue");

    // A l'execution, avec une valeur qui n'est pas une constante :
    u32 exposant = 3U;
    CHECK_EQ(mod06::ipow(10U, exposant), avio::u64{1000U});

    CHECK_EQ(mod06::popcount(0xFFFFU), u32{16});
    CHECK_EQ(mod06::popcount(0U), u32{0});
    CHECK(mod06::even_parity(0x03U));
    CHECK_FALSE(mod06::even_parity(0x07U));
}

TEST_REQ(Constexpr, table_de_linearisation, "LLR-M06-025") {
    CHECK_EQ(mod06::kLinearisation.values[0], avio::i16{-600});
    CHECK_EQ(mod06::kLinearisation.values[15], avio::i16{800});
    // Monotonie de la table : propriete verifiable a l'execution.
    for (usize index = 1U; index < mod06::LinearisationTable::kPointCount; ++index) {
        CHECK(mod06::kLinearisation.values[index] > mod06::kLinearisation.values[index - 1U]);
    }
}

// =============================================================================
//  3. Polymorphisme statique (CRTP)
// =============================================================================
TEST_REQ(CRTP, comportement_identique_au_dynamique, "LLR-M06-030") {
    const mod06::StaticPressureSensor pression;
    const mod06::StaticTemperatureSensor temperature;

    CHECK_NEAR(static_cast<double>(pression.to_engineering(0)), 0.0, 1e-3);
    CHECK_NEAR(static_cast<double>(pression.to_engineering(4095)), 1200.0, 1e-3);
    CHECK_NEAR(static_cast<double>(temperature.to_engineering(0)), -60.0, 1e-3);
    CHECK_NEAR(static_cast<double>(temperature.to_engineering(4095)), 80.0, 1e-3);
}

TEST_REQ(CRTP, aucun_cout_memoire, "LLR-M06-031") {
    // Le point decisif : pas de pointeur de vtable. Comparez avec
    // LLR-M05-021, ou sizeof(PressureSensor) == sizeof(void*).
    CHECK_EQ(sizeof(mod06::StaticPressureSensor), usize{1});
    CHECK(sizeof(mod06::StaticPressureSensor) < sizeof(void*));
}

TEST_REQ(CRTP, comportement_commun_factorise, "LLR-M06-032") {
    const mod06::StaticPressureSensor pression;
    CHECK(pression.is_in_range(0));
    CHECK(pression.is_in_range(4095));
    CHECK_FALSE(pression.is_in_range(-1));

    // to_engineering_clamped est ecrit UNE FOIS dans SensorBase, et disponible
    // pour tous les derives sans aucune duplication.
    CHECK_NEAR(static_cast<double>(pression.to_engineering_clamped(-500)), 0.0, 1e-3);
    CHECK_NEAR(static_cast<double>(pression.to_engineering_clamped(99999)), 1200.0, 1e-3);
}

TEST_REQ(CRTP, fonction_generique_par_instanciation, "LLR-M06-033") {
    const mod06::StaticPressureSensor pression;
    const mod06::StaticTemperatureSensor temperature;
    const i32 echantillons[4] = {0, 1365, 2730, 4095};

    // Deux instanciations distinctes de lire_moyenne : deux codes a couvrir.
    const f32 moyenne_pression = mod06::lire_moyenne(pression, echantillons, 4U);
    const f32 moyenne_temperature = mod06::lire_moyenne(temperature, echantillons, 4U);

    CHECK_NEAR(static_cast<double>(moyenne_pression), 600.0, 1.0);
    CHECK_NEAR(static_cast<double>(moyenne_temperature), 10.0, 1.0);
}

TEST_REQ(CRTP, robustesse_entree_nulle, "LLR-M06-034") {
    const mod06::StaticPressureSensor pression;
    CHECK_NEAR(static_cast<double>(mod06::lire_moyenne(pression, nullptr, 4U)), 0.0, 1e-3);
    const i32 echantillons[1] = {100};
    CHECK_NEAR(static_cast<double>(mod06::lire_moyenne(pression, echantillons, 0U)), 0.0, 1e-3);
}
