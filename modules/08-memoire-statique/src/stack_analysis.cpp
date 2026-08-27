#include "mod08/stack_analysis.hpp"

namespace mod08 {
namespace {

avio::u32 g_depth = 0U;
avio::u32 g_max_depth = 0U;

/// 20! = 2 432 902 008 176 640 000 tient sur 64 bits ; 21! deborde.
constexpr avio::u32 kMaxFactorialInput = 20U;

}  // namespace

// -----------------------------------------------------------------------------
//  CallDepthMonitor
// -----------------------------------------------------------------------------

void CallDepthMonitor::reset() noexcept {
    g_depth = 0U;
    g_max_depth = 0U;
}

avio::u32 CallDepthMonitor::current() noexcept {
    return g_depth;
}

avio::u32 CallDepthMonitor::maximum() noexcept {
    return g_max_depth;
}

CallDepthMonitor::Scope::Scope() noexcept {
    g_depth += 1U;
    if (g_depth > g_max_depth) {
        g_max_depth = g_depth;
    }
}

CallDepthMonitor::Scope::~Scope() noexcept {
    if (g_depth > 0U) {
        g_depth -= 1U;
    }
}

// -----------------------------------------------------------------------------
//  Versions iteratives
// -----------------------------------------------------------------------------

bool factorial(avio::u32 n, avio::u64& out_result) noexcept {
    const CallDepthMonitor::Scope scope;

    if (n > kMaxFactorialInput) {
        out_result = 0U;
        return false;
    }

    avio::u64 accumulateur = 1U;
    for (avio::u32 facteur = 2U; facteur <= n; ++facteur) {
        accumulateur *= static_cast<avio::u64>(facteur);
    }
    out_result = accumulateur;
    return true;
}

avio::i64 sum_iterative(const avio::i32* values, avio::usize count) noexcept {
    if (values == nullptr) {
        return 0;
    }
    avio::i64 total = 0;
    for (avio::usize index = 0U; index < count; ++index) {
        total += static_cast<avio::i64>(values[index]);
    }
    return total;
}

bool binary_search(const avio::i32* sorted_values, avio::usize count, avio::i32 target,
                   avio::usize& out_index) noexcept {
    if ((sorted_values == nullptr) || (count == 0U)) {
        return false;
    }

    avio::usize bas = 0U;
    avio::usize haut = count;  // borne exclusive

    // Le nombre d'iterations est borne par log2(count) : le WCET est calculable
    // a partir de la taille maximale du tableau, elle-meme connue.
    while (bas < haut) {
        const avio::usize milieu = bas + ((haut - bas) / 2U);
        const avio::i32 valeur = sorted_values[milieu];
        if (valeur == target) {
            out_index = milieu;
            return true;
        }
        if (valeur < target) {
            bas = milieu + 1U;
        } else {
            haut = milieu;
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
//  Contre-exemple recursif
// -----------------------------------------------------------------------------

bool factorial_recursive(avio::u32 n, avio::u64& out_result) noexcept {
    const CallDepthMonitor::Scope scope;

    if (n > kMaxFactorialInput) {
        out_result = 0U;
        return false;
    }
    if (n <= 1U) {
        out_result = 1U;
        return true;
    }

    avio::u64 partiel = 0U;
    // Chaque appel consomme une trame de pile supplementaire. Ici la
    // profondeur vaut n, donc au plus 20 : bornee, mais il faut le DEMONTRER
    // et l'inscrire au budget. La version iterative, elle, ne demande rien.
    if (!factorial_recursive(n - 1U, partiel)) {
        out_result = 0U;
        return false;
    }
    out_result = partiel * static_cast<avio::u64>(n);
    return true;
}

}  // namespace mod08
