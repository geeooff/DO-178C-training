#include "mod05/sensors.hpp"

#include <cmath>

namespace mod05 {
namespace {

constexpr avio::i32 kRawMin = 0;
constexpr avio::i32 kRawMax = 4095;

/// Interpolation lineaire brut -> grandeur physique, avec ecretage aux bornes
/// (clause C7 du contrat).
avio::f32 linear_map(avio::i32 raw, avio::i32 raw_min, avio::i32 raw_max, avio::f32 value_min,
                     avio::f32 value_max) noexcept {
    if (raw <= raw_min) {
        return value_min;
    }
    if (raw >= raw_max) {
        return value_max;
    }
    const avio::f32 span_raw = static_cast<avio::f32>(raw_max - raw_min);
    const avio::f32 position = static_cast<avio::f32>(raw - raw_min) / span_raw;
    return value_min + (position * (value_max - value_min));
}

}  // namespace

// -----------------------------------------------------------------------------
//  Sensor
// -----------------------------------------------------------------------------

bool Sensor::is_in_range(avio::i32 raw) const noexcept {
    return (raw >= raw_min()) && (raw <= raw_max());
}

// -----------------------------------------------------------------------------
//  PressureSensor
// -----------------------------------------------------------------------------

const char* PressureSensor::name() const noexcept {
    return "Pression";
}
avio::i32 PressureSensor::raw_min() const noexcept {
    return kRawMin;
}
avio::i32 PressureSensor::raw_max() const noexcept {
    return kRawMax;
}
avio::f32 PressureSensor::value_min() const noexcept {
    return 0.0F;
}
avio::f32 PressureSensor::value_max() const noexcept {
    return 1200.0F;
}
avio::f32 PressureSensor::to_engineering(avio::i32 raw) const noexcept {
    return linear_map(raw, kRawMin, kRawMax, 0.0F, 1200.0F);
}

// -----------------------------------------------------------------------------
//  TemperatureSensor
// -----------------------------------------------------------------------------

const char* TemperatureSensor::name() const noexcept {
    return "Temperature";
}
avio::i32 TemperatureSensor::raw_min() const noexcept {
    return kRawMin;
}
avio::i32 TemperatureSensor::raw_max() const noexcept {
    return kRawMax;
}
avio::f32 TemperatureSensor::value_min() const noexcept {
    return -60.0F;
}
avio::f32 TemperatureSensor::value_max() const noexcept {
    return 80.0F;
}
avio::f32 TemperatureSensor::to_engineering(avio::i32 raw) const noexcept {
    return linear_map(raw, kRawMin, kRawMax, -60.0F, 80.0F);
}

// -----------------------------------------------------------------------------
//  BrokenSensor -- violations volontaires
// -----------------------------------------------------------------------------

const char* BrokenSensor::name() const noexcept {
    return "CapteurFautif";
}
avio::i32 BrokenSensor::raw_min() const noexcept {
    return kRawMin;
}
avio::i32 BrokenSensor::raw_max() const noexcept {
    return kRawMax;
}
avio::f32 BrokenSensor::value_min() const noexcept {
    return 0.0F;
}
avio::f32 BrokenSensor::value_max() const noexcept {
    return 1200.0F;
}

avio::f32 BrokenSensor::to_engineering(avio::i32 raw) const noexcept {
    // VIOLATION C7 : aucune borne pour les entrees hors domaine.
    // VIOLATION C5 : la courbe redescend au-dela de 3000 -- "optimisation"
    // typique d'une correction ajoutee apres coup sans revoir le contrat.
    if (raw > 3000) {
        return 1200.0F - (static_cast<avio::f32>(raw - 3000) * 0.5F);
    }
    return static_cast<avio::f32>(raw) * (1200.0F / 4095.0F);
}

// -----------------------------------------------------------------------------
//  ContractReport
// -----------------------------------------------------------------------------

bool ContractReport::all_satisfied() const noexcept {
    return c1_name_valid && c2_raw_domain_valid && c3_value_domain_valid && c4_result_in_domain &&
           c5_monotonic && c6_endpoints_match && c7_clamped_outside;
}

const char* ContractReport::first_violation() const noexcept {
    if (!c1_name_valid) {
        return "C1";
    }
    if (!c2_raw_domain_valid) {
        return "C2";
    }
    if (!c3_value_domain_valid) {
        return "C3";
    }
    if (!c4_result_in_domain) {
        return "C4";
    }
    if (!c5_monotonic) {
        return "C5";
    }
    if (!c6_endpoints_match) {
        return "C6";
    }
    if (!c7_clamped_outside) {
        return "C7";
    }
    return nullptr;
}

// -----------------------------------------------------------------------------
//  verify_contract -- le "test pessimiste" de la DO-332
// -----------------------------------------------------------------------------

ContractReport verify_contract(const Sensor& sensor, avio::u32 sample_count) noexcept {
    ContractReport report;

    const avio::u32 samples = (sample_count < 2U) ? 2U : sample_count;
    constexpr avio::f32 kTolerance = 0.01F;

    // C1 : nom valide
    const char* name_text = sensor.name();
    report.c1_name_valid = (name_text != nullptr) && (name_text[0] != '\0');

    // C2 / C3 : domaines coherents
    const avio::i32 raw_min = sensor.raw_min();
    const avio::i32 raw_max = sensor.raw_max();
    const avio::f32 value_min = sensor.value_min();
    const avio::f32 value_max = sensor.value_max();
    report.c2_raw_domain_valid = (raw_min < raw_max);
    report.c3_value_domain_valid = (value_min < value_max);

    if (!report.c2_raw_domain_valid || !report.c3_value_domain_valid) {
        return report;  // inutile de poursuivre sur un domaine incoherent
    }

    // C4 / C5 : parcours du domaine
    bool in_domain = true;
    bool monotonic = true;
    avio::f32 previous = value_min;

    const avio::i64 span_size = static_cast<avio::i64>(raw_max) - static_cast<avio::i64>(raw_min);
    for (avio::u32 index = 0U; index < samples; ++index) {
        const avio::i64 offset =
            (span_size * static_cast<avio::i64>(index)) / static_cast<avio::i64>(samples - 1U);
        const avio::i32 raw = static_cast<avio::i32>(static_cast<avio::i64>(raw_min) + offset);
        const avio::f32 value = sensor.to_engineering(raw);

        if (!std::isfinite(value) || (value < (value_min - kTolerance)) ||
            (value > (value_max + kTolerance))) {
            in_domain = false;
        }
        if (index > 0U) {
            if (value < (previous - kTolerance)) {
                monotonic = false;
            }
        }
        previous = value;
    }
    report.c4_result_in_domain = in_domain;
    report.c5_monotonic = monotonic;

    // C6 : correspondance aux extremites
    const avio::f32 value_at_min = sensor.to_engineering(raw_min);
    const avio::f32 value_at_max = sensor.to_engineering(raw_max);
    report.c6_endpoints_match = (std::fabs(value_at_min - value_min) <= kTolerance) &&
                                (std::fabs(value_at_max - value_max) <= kTolerance);

    // C7 : ecretage hors domaine (robustesse)
    const avio::f32 below_domain = sensor.to_engineering(raw_min - 1000);
    const avio::f32 above_domain = sensor.to_engineering(raw_max + 1000);
    report.c7_clamped_outside = std::isfinite(below_domain) && std::isfinite(above_domain) &&
                                (std::fabs(below_domain - value_min) <= kTolerance) &&
                                (std::fabs(above_domain - value_max) <= kTolerance);

    return report;
}

// -----------------------------------------------------------------------------
//  Decoupage
// -----------------------------------------------------------------------------

// DEVIATION JUSTIFIEE (performance-unnecessary-value-param) : clang-tidy
// recommande ici `const Message&` -- et il a raison, c'est exactement la
// PARADE au decoupage. Mais le passage par valeur est le DEFAUT que cette
// fonction doit demontrer (LLR-M05-030). Derogation locale et tracee.
// NOLINTNEXTLINE(performance-unnecessary-value-param)
avio::u16 length_by_value(Message message) noexcept {
    // `message` est une COPIE de type statique Message : la partie derivee a
    // ete perdue a l'entree de la fonction. L'appel virtuel resout donc
    // toujours vers Message::length().
    return message.length();
}

avio::u16 length_by_reference(const Message& message) noexcept {
    // Aucune copie : le type dynamique est intact, la resolution est correcte.
    return message.length();
}

}  // namespace mod05
