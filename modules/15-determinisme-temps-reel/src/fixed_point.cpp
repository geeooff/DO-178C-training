#include "mod15/fixed_point.hpp"

#include <cmath>

namespace mod15 {
namespace {

/// Sature un resultat 64 bits vers le domaine 32 bits signe.
avio::i32 saturate(avio::i64 value) noexcept {
    if (value > static_cast<avio::i64>(Fixed::kRawMax)) {
        return Fixed::kRawMax;
    }
    if (value < static_cast<avio::i64>(Fixed::kRawMin)) {
        return Fixed::kRawMin;
    }
    return static_cast<avio::i32>(value);
}

}  // namespace

/// @satisfies LLR-FIX-010
Fixed Fixed::from_int(avio::i32 value) noexcept {
    const avio::i64 raw_value = static_cast<avio::i64>(value) * static_cast<avio::i64>(kOne);
    return Fixed::from_raw(saturate(raw_value));
}

/// @satisfies LLR-FIX-011
Fixed Fixed::from_float(avio::f32 value) noexcept {
    // NaN n'a AUCUN equivalent en virgule fixe, pas meme approximatif : on
    // renvoie zero, valeur neutre et deterministe. Les infinis, eux, ont un
    // equivalent naturel : les bornes du domaine.
    // Le rejet A LA FRONTIERE (module 04) reste la bonne pratique ; ici on
    // garantit seulement l'absence de comportement indefini.
    if (std::isnan(value)) {
        return Fixed();
    }

    const double raw_value = static_cast<double>(value) * static_cast<double>(kOne);
    if (raw_value > static_cast<double>(kRawMax)) {
        return Fixed::from_raw(kRawMax);
    }
    if (raw_value < static_cast<double>(kRawMin)) {
        return Fixed::from_raw(kRawMin);
    }

    // Arrondi au plus proche, en s'eloignant de zero. On l'ecrit a la main
    // plutot que d'appeler std::lround : pas de dependance au mode d'arrondi
    // courant, pas d'appel de bibliotheque, et un WCET trivial.
    // L'erreur de conversion est bornee par kResolution / 2, soit 7,6e-6.
    // Cette borne est CALCULABLE avant execution : c'est tout l'interet de la
    // virgule fixe.
    const double rounded = (raw_value >= 0.0) ? (raw_value + 0.5) : (raw_value - 0.5);
    return Fixed::from_raw(static_cast<avio::i32>(rounded));
}

/// @satisfies LLR-FIX-012
avio::i32 Fixed::to_int() const noexcept {
    // Decalage arithmetique vers la droite : troncature vers MOINS L'INFINI,
    // pas vers zero. -1,5 donne -2. Ce choix est SPECIFIE, pas subi.
    return raw_ >> kFractionBits;
}

avio::f32 Fixed::to_float() const noexcept {
    return static_cast<avio::f32>(raw_) / static_cast<avio::f32>(kOne);
}

/// @satisfies LLR-FIX-020
Fixed Fixed::operator+(Fixed other) const noexcept {
    return Fixed::from_raw(
        saturate(static_cast<avio::i64>(raw_) + static_cast<avio::i64>(other.raw_)));
}

/// @satisfies LLR-FIX-021
Fixed Fixed::operator-(Fixed other) const noexcept {
    return Fixed::from_raw(
        saturate(static_cast<avio::i64>(raw_) - static_cast<avio::i64>(other.raw_)));
}

/// @satisfies LLR-FIX-022
Fixed Fixed::operator*(Fixed other) const noexcept {
    // Le produit de deux Q16.16 est un Q32.32 : il faut donc 64 bits pendant
    // le calcul, puis un recalage de 16 bits.
    const avio::i64 product = static_cast<avio::i64>(raw_) * static_cast<avio::i64>(other.raw_);
    return Fixed::from_raw(saturate(product >> kFractionBits));
}

/// @satisfies LLR-FIX-023
bool Fixed::divide(Fixed divisor, Fixed& out) const noexcept {
    if (divisor.raw_ == 0) {
        out = Fixed();
        return false;
    }
    const avio::i64 numerator = static_cast<avio::i64>(raw_) << kFractionBits;
    out = Fixed::from_raw(saturate(numerator / static_cast<avio::i64>(divisor.raw_)));
    return true;
}

Fixed Fixed::operator-() const noexcept {
    return Fixed::from_raw(saturate(-static_cast<avio::i64>(raw_)));
}

/// @satisfies LLR-FIX-030
Fixed accumulate(Fixed increment, avio::u32 count) noexcept {
    Fixed total;
    for (avio::u32 index = 0U; index < count; ++index) {
        total = total + increment;
    }
    return total;
}

}  // namespace mod15
