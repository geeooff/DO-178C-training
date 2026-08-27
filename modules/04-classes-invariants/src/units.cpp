#include "mod04/units.hpp"

#include <cmath>

namespace mod04 {
namespace {

constexpr avio::f32 kGramsPerKilogram = 1000.0F;
constexpr avio::f32 kGramsPerPound = 453.59237F;  // definition legale exacte

/// Conversion flottant -> entier avec verification complete du domaine.
/// Reprend la lecon du module 01 (Ariane 5) appliquee aux flottants.
bool to_grams(avio::f32 value, avio::i32& out_grams) noexcept {
    if (!std::isfinite(value)) {
        return false;  // NaN ou infini : entree invalide
    }
    if (value < 0.0F) {
        return false;  // une masse negative n'a pas de sens physique
    }
    if (value > static_cast<avio::f32>(Mass::kMaxGrams)) {
        return false;
    }
    out_grams = static_cast<avio::i32>(value);
    return true;
}

Mass smaller_of(Mass lhs, Mass rhs) noexcept {
    return (lhs < rhs) ? lhs : rhs;
}

}  // namespace

// -----------------------------------------------------------------------------
//  Mass
// -----------------------------------------------------------------------------

bool Mass::from_grams(avio::i32 grams, Mass& out) noexcept {
    if ((grams < 0) || (grams > kMaxGrams)) {
        return false;
    }
    out = Mass(grams);
    return true;
}

bool Mass::from_kilograms(avio::f32 kilograms, Mass& out) noexcept {
    avio::i32 grams = 0;
    if (!to_grams(kilograms * kGramsPerKilogram, grams)) {
        return false;
    }
    out = Mass(grams);
    return true;
}

bool Mass::from_pounds(avio::f32 pounds, Mass& out) noexcept {
    avio::i32 grams = 0;
    if (!to_grams(pounds * kGramsPerPound, grams)) {
        return false;
    }
    out = Mass(grams);
    return true;
}

avio::f32 Mass::kilograms() const noexcept {
    return static_cast<avio::f32>(grams_) / kGramsPerKilogram;
}

avio::f32 Mass::pounds() const noexcept {
    return static_cast<avio::f32>(grams_) / kGramsPerPound;
}

Mass Mass::operator+(Mass other) const noexcept {
    // Calcul en 64 bits : impossible de deborder pendant l'operation.
    const avio::i64 sum = static_cast<avio::i64>(grams_) + static_cast<avio::i64>(other.grams_);
    if (sum > static_cast<avio::i64>(kMaxGrams)) {
        return Mass(kMaxGrams);
    }
    return Mass(static_cast<avio::i32>(sum));
}

Mass Mass::operator-(Mass other) const noexcept {
    const avio::i64 difference =
        static_cast<avio::i64>(grams_) - static_cast<avio::i64>(other.grams_);
    if (difference < 0) {
        return Mass(0);  // une masse reste positive : borne basse a zero
    }
    return Mass(static_cast<avio::i32>(difference));
}

// -----------------------------------------------------------------------------
//  Altitude
// -----------------------------------------------------------------------------

bool Altitude::from_feet(avio::f32 feet, Altitude& out) noexcept {
    if (!std::isfinite(feet)) {
        return false;
    }
    if ((feet < kMinFeet) || (feet > kMaxFeet)) {
        return false;
    }
    out = Altitude(feet);
    return true;
}

bool Altitude::from_meters(avio::f32 meters, Altitude& out) noexcept {
    if (!std::isfinite(meters)) {
        return false;
    }
    return from_feet(meters * kFeetPerMeter, out);
}

avio::f32 Altitude::meters() const noexcept {
    return feet_ / kFeetPerMeter;
}

bool Altitude::is_close(Altitude other, avio::f32 tolerance_feet) const noexcept {
    if (!std::isfinite(tolerance_feet) || (tolerance_feet < 0.0F)) {
        return false;  // robustesse : tolerance absurde
    }
    return std::fabs(feet_ - other.feet_) <= tolerance_feet;
}

avio::f32 Altitude::difference_feet(Altitude other) const noexcept {
    return feet_ - other.feet_;
}

// -----------------------------------------------------------------------------
//  FuelTank
// -----------------------------------------------------------------------------

bool FuelTank::create(Mass capacity, FuelTank& out) noexcept {
    Mass zero;
    if (capacity == zero) {
        return false;
    }
    out = FuelTank(capacity, zero);
    return true;
}

avio::f32 FuelTank::fill_ratio_percent() const noexcept {
    if (capacity_.grams() == 0) {
        // ROBUSTESSE : reservoir degenere. On ne divise jamais par zero, et on
        // renvoie une valeur bornee plutot qu'un NaN qui se propagerait dans
        // tous les calculs en aval.
        return 0.0F;
    }
    const avio::f32 ratio =
        (static_cast<avio::f32>(quantity_.grams()) * 100.0F) /
        static_cast<avio::f32>(capacity_.grams());
    return ratio;
}

Mass FuelTank::add(Mass amount) noexcept {
    const Mass espace_libre = capacity_ - quantity_;
    const Mass ajoute = smaller_of(amount, espace_libre);
    quantity_ = quantity_ + ajoute;
    return ajoute;
}

Mass FuelTank::remove(Mass amount) noexcept {
    const Mass preleve = smaller_of(amount, quantity_);
    quantity_ = quantity_ - preleve;
    return preleve;
}

bool FuelTank::is_empty() const noexcept {
    return quantity_.grams() == 0;
}

bool FuelTank::is_full() const noexcept {
    return quantity_ == capacity_;
}

bool FuelTank::invariant_holds() const noexcept {
    const Mass zero;
    return (quantity_ >= zero) && (quantity_ <= capacity_);
}

}  // namespace mod04
