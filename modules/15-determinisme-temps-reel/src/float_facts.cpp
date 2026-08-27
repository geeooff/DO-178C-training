#include "mod15/float_facts.hpp"

#include <cmath>
#include <cstring>

namespace mod15 {

/// @satisfies LLR-DET-010
bool is_absorbed(avio::f32 x, avio::f32 increment) noexcept {
    // Volatile : empeche le compilateur de conserver le resultat dans un
    // registre plus large que 32 bits, ce qui fausserait la demonstration sur
    // les cibles utilisant encore la pile x87.
    volatile avio::f32 somme = x + increment;
    return static_cast<avio::f32>(somme) == x;
}

/// @satisfies LLR-DET-011
avio::f32 ulp(avio::f32 value) noexcept {
    if (!std::isfinite(value)) {
        return 0.0F;
    }
    // std::nextafter donne le flottant immediatement superieur : la difference
    // est l'ULP (Unit in the Last Place).
    const avio::f32 suivant = std::nextafter(value, 3.4e38F);
    return suivant - value;
}

/// @satisfies LLR-DET-012
bool addition_is_non_associative(avio::f32 a, avio::f32 b, avio::f32 c) noexcept {
    volatile avio::f32 gauche = (a + b) + c;
    volatile avio::f32 droite = a + (b + c);
    return static_cast<avio::f32>(gauche) != static_cast<avio::f32>(droite);
}

/// @satisfies LLR-DET-013
avio::f32 accumulate_float(avio::f32 increment, avio::u32 count) noexcept {
    avio::f32 total = 0.0F;
    for (avio::u32 index = 0U; index < count; ++index) {
        total += increment;
    }
    return total;
}

/// @satisfies LLR-DET-014
avio::f32 accumulate_kahan(avio::f32 increment, avio::u32 count) noexcept {
    avio::f32 total = 0.0F;
    avio::f32 compensation = 0.0F;

    for (avio::u32 index = 0U; index < count; ++index) {
        // Chaque etape recupere l'erreur d'arrondi perdue a l'etape
        // precedente. Les `volatile` sont ESSENTIELS : sans eux, un
        // compilateur autorise a reordonner (par exemple avec /fp:fast)
        // simplifierait algebriquement tout l'algorithme en `total += y`,
        // annulant la compensation.
        volatile avio::f32 y = increment - compensation;
        volatile avio::f32 t = total + y;
        compensation = (static_cast<avio::f32>(t) - total) - static_cast<avio::f32>(y);
        total = static_cast<avio::f32>(t);
    }
    return total;
}

/// @satisfies LLR-DET-020
bool close_absolute(avio::f32 a, avio::f32 b, avio::f32 tolerance) noexcept {
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(tolerance) || (tolerance < 0.0F)) {
        return false;
    }
    return std::fabs(a - b) <= tolerance;
}

/// @satisfies LLR-DET-021
bool close_relative(avio::f32 a, avio::f32 b, avio::f32 relative_tolerance) noexcept {
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(relative_tolerance) ||
        (relative_tolerance < 0.0F)) {
        return false;
    }
    const avio::f32 ecart = std::fabs(a - b);
    const avio::f32 magnitude_a = std::fabs(a);
    const avio::f32 magnitude_b = std::fabs(b);
    const avio::f32 reference = (magnitude_a > magnitude_b) ? magnitude_a : magnitude_b;

    // Cas particulier : les deux valeurs sont nulles ou quasi nulles. La
    // tolerance relative n'a alors plus de sens ; on retombe sur l'egalite.
    if (reference == 0.0F) {
        return ecart == 0.0F;
    }
    return (ecart / reference) <= relative_tolerance;
}

}  // namespace mod15
