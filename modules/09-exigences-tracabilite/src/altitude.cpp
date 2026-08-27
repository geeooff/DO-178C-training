#include "mod09/altitude.hpp"

#include <cmath>

namespace mod09 {
namespace {

/// Pieds par hectopascal au voisinage du niveau de la mer.
/// @satisfies LLR-ADCALT-033
constexpr avio::f32 kFeetPerHpa = 27.0F;

}  // namespace

/// @satisfies LLR-ADCALT-010
Status validate_static_pressure(avio::f32 pressure_hpa) noexcept {
    if (!std::isfinite(pressure_hpa)) {
        return Status::InvalidArgument;
    }
    if ((pressure_hpa < kStaticPressureMinHpa) || (pressure_hpa > kStaticPressureMaxHpa)) {
        return Status::OutOfRange;
    }
    return Status::Ok;
}

/// @satisfies LLR-ADCALT-030
Status validate_qnh(avio::f32 qnh_hpa) noexcept {
    if (!std::isfinite(qnh_hpa)) {
        return Status::InvalidArgument;
    }
    if ((qnh_hpa < kQnhMinHpa) || (qnh_hpa > kQnhMaxHpa)) {
        return Status::OutOfRange;
    }
    return Status::Ok;
}

/// @satisfies LLR-ADCALT-020
/// @satisfies LLR-ADCALT-021
/// @satisfies LLR-ADCALT-022
/// @satisfies LLR-ADCALT-023
Result<avio::f32> pressure_altitude_feet(avio::f32 static_pressure_hpa) noexcept {
    // LLR-ADCALT-021 : validation AVANT tout calcul, statut propage tel quel.
    const Status validation = validate_static_pressure(static_pressure_hpa);
    if (validation != Status::Ok) {
        return Result<avio::f32>::error(validation);
    }

    // LLR-ADCALT-020 : forme fermee du modele ISA.
    const double ratio =
        static_cast<double>(static_pressure_hpa) / static_cast<double>(kIsaSeaLevelPressureHpa);
    const double factor = std::pow(ratio, static_cast<double>(kIsaExponent));
    const double altitude = static_cast<double>(kIsaAltitudeCoefficientFt) * (1.0 - factor);

    // Le calcul intermediaire est fait en double : l'erreur d'arrondi reste
    // tres inferieure au budget de 20 ft (HLR-ADCALT-004). Le resultat est
    // ramene en f32, format de l'interface. Ce choix est une DECISION, pas un
    // hasard : voir module 15 sur le determinisme flottant.
    return Result<avio::f32>::ok(static_cast<avio::f32>(altitude));
}

/// @satisfies LLR-ADCALT-031
/// @satisfies LLR-ADCALT-032
Result<avio::f32> corrected_altitude_feet(avio::f32 static_pressure_hpa,
                                          avio::f32 qnh_hpa) noexcept {
    // LLR-ADCALT-032 : ORDRE DE VALIDATION SPECIFIE. Si les deux entrees sont
    // invalides, c'est le statut de la pression qui remonte. Sans cette
    // specification, le comportement dependrait de l'implementation -- donc
    // ne serait pas verifiable.
    const Result<avio::f32> pressure_altitude = pressure_altitude_feet(static_pressure_hpa);
    if (pressure_altitude.is_error()) {
        return pressure_altitude;
    }

    const Status qnh_validation = validate_qnh(qnh_hpa);
    if (qnh_validation != Status::Ok) {
        return Result<avio::f32>::error(qnh_validation);
    }

    // LLR-ADCALT-031
    const avio::f32 correction = (qnh_hpa - kIsaSeaLevelPressureHpa) * kFeetPerHpa;
    return Result<avio::f32>::ok(pressure_altitude.value() + correction);
}

/// @satisfies LLR-ADCALT-033
avio::f32 feet_per_hpa() noexcept {
    return kFeetPerHpa;
}

}  // namespace mod09
