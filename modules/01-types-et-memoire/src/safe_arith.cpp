#include "mod01/safe_arith.hpp"

namespace mod01 {

bool is_valid(SensorId id) noexcept {
    return to_underlying(id) < to_underlying(SensorId::Count);
}

const char* name_of(SensorId id) noexcept {
    // Un `switch` sur enum class impose au compilateur de signaler les membres
    // oublies (avertissement C4062/C4061 sous MSVC, -Wswitch sous GCC/Clang).
    // C'est une aide precieuse pour la maintenance : ajouter un capteur
    // provoque un avertissement partout ou il faut traiter le nouveau cas.
    switch (id) {
        case SensorId::LeftPitot:
            return "PitotGauche";
        case SensorId::RightPitot:
            return "PitotDroit";
        case SensorId::StandbyPitot:
            return "PitotSecours";
        case SensorId::StaticPort:
            return "PriseStatique";
        case SensorId::Count:
        default:
            // Cas de ROBUSTESSE : valeur hors domaine obtenue par un cast.
            // On ne renvoie jamais nullptr : l'appelant n'aurait pas de moyen
            // sur de s'en apercevoir.
            return "Inconnu";
    }
}

bool checked_add(avio::i32 lhs, avio::i32 rhs, avio::i32& result) noexcept {
    constexpr avio::i32 kMin = std::numeric_limits<avio::i32>::min();
    constexpr avio::i32 kMax = std::numeric_limits<avio::i32>::max();

    // On teste AVANT de calculer : effectuer l'addition puis constater le
    // debordement serait deja un comportement indefini.
    if ((rhs > 0) && (lhs > (kMax - rhs))) {
        result = 0;
        return false;
    }
    if ((rhs < 0) && (lhs < (kMin - rhs))) {
        result = 0;
        return false;
    }
    result = lhs + rhs;
    return true;
}

bool checked_div(avio::i32 numerator, avio::i32 denominator, avio::i32& result) noexcept {
    constexpr avio::i32 kMin = std::numeric_limits<avio::i32>::min();

    if (denominator == 0) {
        result = 0;
        return false;
    }
    // Seul cas de debordement de la division signee : la valeur minimale
    // divisee par -1 vaut 2147483648, qui n'est pas representable sur i32.
    if ((numerator == kMin) && (denominator == -1)) {
        result = 0;
        return false;
    }
    result = numerator / denominator;
    return true;
}

}  // namespace mod01
