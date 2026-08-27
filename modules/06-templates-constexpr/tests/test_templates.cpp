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

TEST_REQ(RingBuffer_i32_4, complete_life_cycle, "LLR-M06-001") {
    mod06::RingBuffer<i32, 4U> buffer;

    CHECK(buffer.empty());
    CHECK_FALSE(buffer.full());
    CHECK_EQ(buffer.size(), usize{0});
    CHECK_EQ(buffer.kCapacity, usize{4});

    CHECK(buffer.push(10));
    CHECK(buffer.push(20));
    CHECK(buffer.push(30));
    CHECK(buffer.push(40));
    CHECK(buffer.full());
    CHECK_EQ(buffer.overwrite_count(), u32{0});

    // Cinquieme element : le plus ancien (10) est ecrase.
    CHECK_FALSE(buffer.push(50));
    CHECK_EQ(buffer.overwrite_count(), u32{1});
    CHECK_EQ(buffer.size(), usize{4});

    i32 value = 0;
    REQUIRE(buffer.pop(value));
    CHECK_EQ(value, 20);  // 10 a disparu
    REQUIRE(buffer.peek(0U, value));
    CHECK_EQ(value, 30);
}

TEST_REQ(RingBuffer_i32_4, robustness_empty_buffer, "LLR-M06-002") {
    mod06::RingBuffer<i32, 4U> buffer;
    i32 value = 999;
    CHECK_FALSE(buffer.pop(value));
    CHECK_FALSE(buffer.peek(0U, value));
    CHECK_EQ(value, 999);  // sortie non modifiee
}

TEST_REQ(RingBuffer_i32_4, reset_to_zero, "LLR-M06-003") {
    mod06::RingBuffer<i32, 4U> buffer;
    for (i32 index = 0; index < 10; ++index) {
        (void)buffer.push(index);
    }
    CHECK(buffer.overwrite_count() > 0U);
    buffer.clear();
    CHECK(buffer.empty());
    CHECK_EQ(buffer.overwrite_count(), u32{0});
}

TEST_REQ(RingBuffer_f32_8, float_instantiation, "LLR-M06-004") {
    // MEME code source, AUTRE code executable : il faut le couvrir aussi.
    mod06::RingBuffer<f32, 8U> buffer;
    CHECK_EQ(buffer.kCapacity, usize{8});

    for (u32 index = 0U; index < 8U; ++index) {
        CHECK(buffer.push(static_cast<f32>(index) * 1.5F));
    }
    CHECK(buffer.full());

    f32 value = 0.0F;
    REQUIRE(buffer.peek(2U, value));
    CHECK_NEAR(static_cast<double>(value), 3.0, 1e-6);
}

TEST_REQ(RingBuffer_u8_16, byte_instantiation, "LLR-M06-005") {
    mod06::RingBuffer<u8, 16U> buffer;
    CHECK_EQ(buffer.kCapacity, usize{16});
    for (u32 index = 0U; index < 20U; ++index) {
        (void)buffer.push(static_cast<u8>(index));
    }
    CHECK_EQ(buffer.overwrite_count(), u32{4});
    u8 value = 0U;
    REQUIRE(buffer.peek(0U, value));
    CHECK_EQ(value, u8{4U});  // 0..3 ont ete ecrases
}

TEST_REQ(RingBuffer, memory_size_per_instantiation, "LLR-M06-006") {
    // La taille depend des parametres : c'est une donnee d'architecture a
    // documenter (budget RAM par instanciation).
    CHECK(sizeof(mod06::RingBuffer<i32, 4U>) >= (4U * sizeof(i32)));
    CHECK(sizeof(mod06::RingBuffer<u8, 16U>) >= 16U);
    CHECK(sizeof(mod06::RingBuffer<i32, 4U>) != sizeof(mod06::RingBuffer<f32, 8U>));
}

// -----------------------------------------------------------------------------
//  average : les deux branches de `if constexpr` sont DEUX codes distincts
// -----------------------------------------------------------------------------
TEST_REQ(Average, integer_branch, "LLR-M06-010") {
    mod06::RingBuffer<i32, 4U> buffer;
    (void)buffer.push(10);
    (void)buffer.push(20);
    (void)buffer.push(31);
    // Moyenne entiere : troncature vers zero, 61/3 = 20
    CHECK_EQ(mod06::average(buffer), 20);
}

TEST_REQ(Average, float_branch, "LLR-M06-011") {
    mod06::RingBuffer<f32, 8U> buffer;
    (void)buffer.push(1.0F);
    (void)buffer.push(2.0F);
    (void)buffer.push(4.0F);
    CHECK_NEAR(static_cast<double>(mod06::average(buffer)), 2.3333333, 1e-5);
}

TEST_REQ(Average, robustness_empty_buffer, "LLR-M06-012") {
    const mod06::RingBuffer<i32, 4U> integer;
    const mod06::RingBuffer<f32, 8U> float_buffer;
    CHECK_EQ(mod06::average(integer), 0);
    CHECK_NEAR(static_cast<double>(mod06::average(float_buffer)), 0.0, 1e-9);
}

// =============================================================================
//  2. constexpr
// =============================================================================
TEST_REQ(Constexpr, crc_table_in_rom, "LLR-M06-020") {
    // Ces valeurs ont deja ete verifiees par static_assert A LA COMPILATION.
    // Les retester a l'execution montre que la table embarquee est bien celle
    // que le compilateur a calculee.
    CHECK_EQ(mod06::kCrc8Table.values[0], u8{0x00U});
    CHECK_EQ(mod06::kCrc8Table.values[1], u8{0x07U});
    CHECK_EQ(mod06::kCrc8Table.values[255], u8{0xF3U});
}

TEST_REQ(Constexpr, crc8_reference_value, "LLR-M06-021") {
    // Vecteur de test standard du CRC-8/SMBUS : la chaine "123456789".
    const u8 message[9] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U};
    CHECK_EQ(mod06::crc8(avio::make_const_span(message)), u8{0xF4U});
}

TEST_REQ(Constexpr, crc8_detects_alteration, "LLR-M06-022") {
    u8 frame[4] = {0x12U, 0x34U, 0x56U, 0x78U};
    const u8 reference = mod06::crc8(avio::make_const_span(frame));
    CHECK_EQ(reference, u8{0x1CU});

    frame[1] = 0x35U;  // un seul bit change
    CHECK(mod06::crc8(avio::make_const_span(frame)) != reference);
}

TEST_REQ(Constexpr, crc8_empty_buffer, "LLR-M06-023") {
    const avio::Span<const u8> empty;
    CHECK_EQ(mod06::crc8(empty, 0x5AU), u8{0x5AU});
}

TEST_REQ(Constexpr, functions_usable_at_both_times, "LLR-M06-024") {
    // A la compilation :
    constexpr avio::u64 thousand_twenty_four = mod06::ipow(2U, 10U);
    static_assert(thousand_twenty_four == 1024U, "evaluation a la compilation attendue");

    // A l'execution, avec une valeur qui n'est pas une constante :
    u32 exponent = 3U;
    CHECK_EQ(mod06::ipow(10U, exponent), avio::u64{1000U});

    CHECK_EQ(mod06::popcount(0xFFFFU), u32{16});
    CHECK_EQ(mod06::popcount(0U), u32{0});
    CHECK(mod06::even_parity(0x03U));
    CHECK_FALSE(mod06::even_parity(0x07U));
}

TEST_REQ(Constexpr, linearization_table, "LLR-M06-025") {
    CHECK_EQ(mod06::kLinearization.values[0], avio::i16{-600});
    CHECK_EQ(mod06::kLinearization.values[15], avio::i16{800});
    // Monotonie de la table : propriete verifiable a l'execution.
    for (usize index = 1U; index < mod06::LinearizationTable::kPointCount; ++index) {
        CHECK(mod06::kLinearization.values[index] > mod06::kLinearization.values[index - 1U]);
    }
}

// =============================================================================
//  3. Polymorphisme statique (CRTP)
// =============================================================================
TEST_REQ(CRTP, behavior_identical_to_dynamic, "LLR-M06-030") {
    const mod06::StaticPressureSensor pressure;
    const mod06::StaticTemperatureSensor temperature;

    CHECK_NEAR(static_cast<double>(pressure.to_engineering(0)), 0.0, 1e-3);
    CHECK_NEAR(static_cast<double>(pressure.to_engineering(4095)), 1200.0, 1e-3);
    CHECK_NEAR(static_cast<double>(temperature.to_engineering(0)), -60.0, 1e-3);
    CHECK_NEAR(static_cast<double>(temperature.to_engineering(4095)), 80.0, 1e-3);
}

TEST_REQ(CRTP, no_memory_cost, "LLR-M06-031") {
    // Le point decisif : pas de pointeur de vtable. Comparez avec
    // LLR-M05-021, ou sizeof(PressureSensor) == sizeof(void*).
    CHECK_EQ(sizeof(mod06::StaticPressureSensor), usize{1});
    CHECK(sizeof(mod06::StaticPressureSensor) < sizeof(void*));
}

TEST_REQ(CRTP, factored_common_behavior, "LLR-M06-032") {
    const mod06::StaticPressureSensor pressure;
    CHECK(pressure.is_in_range(0));
    CHECK(pressure.is_in_range(4095));
    CHECK_FALSE(pressure.is_in_range(-1));

    // to_engineering_clamped est ecrit UNE FOIS dans SensorBase, et disponible
    // pour tous les derives sans aucune duplication.
    CHECK_NEAR(static_cast<double>(pressure.to_engineering_clamped(-500)), 0.0, 1e-3);
    CHECK_NEAR(static_cast<double>(pressure.to_engineering_clamped(99999)), 1200.0, 1e-3);
}

TEST_REQ(CRTP, generic_function_per_instantiation, "LLR-M06-033") {
    const mod06::StaticPressureSensor pressure;
    const mod06::StaticTemperatureSensor temperature;
    const i32 samples[4] = {0, 1365, 2730, 4095};

    // Deux instanciations distinctes de read_average : deux codes a couvrir.
    const f32 average_pressure = mod06::read_average(pressure, samples, 4U);
    const f32 average_temperature = mod06::read_average(temperature, samples, 4U);

    CHECK_NEAR(static_cast<double>(average_pressure), 600.0, 1.0);
    CHECK_NEAR(static_cast<double>(average_temperature), 10.0, 1.0);
}

TEST_REQ(CRTP, robustness_null_input, "LLR-M06-034") {
    const mod06::StaticPressureSensor pressure;
    CHECK_NEAR(static_cast<double>(mod06::read_average(pressure, nullptr, 4U)), 0.0, 1e-3);
    const i32 samples[1] = {100};
    CHECK_NEAR(static_cast<double>(mod06::read_average(pressure, samples, 0U)), 0.0, 1e-3);
}
