// =============================================================================
//  Module 01 -- arithmetique sure et conversions maitrisees.
//
//  Contexte DO-178C : le debordement arithmetique et les conversions
//  implicites sont dans le peloton de tete des causes d'anomalie en embarque.
//  L'echec du vol inaugural d'Ariane 5 (1996) tient en une ligne : conversion
//  d'un flottant 64 bits vers un entier signe 16 bits, sans protection, dans
//  du code reutilise d'Ariane 4 hors de son domaine de validite.
//
//  La table A-5 de la DO-178C demande de verifier que le code source est
//  "accurate and consistent" : cela couvre explicitement le debordement,
//  la resolution, et l'usage correct des types. Les regles MISRA C++ associees
//  interdisent les conversions implicites qui perdent de l'information.
//
//  Ce module fournit les briques qui permettent de RESPECTER ces regles.
// =============================================================================
#ifndef MOD01_SAFE_ARITH_HPP
#define MOD01_SAFE_ARITH_HPP

#include <avio/types.hpp>
#include <limits>
#include <type_traits>

namespace mod01 {

// -----------------------------------------------------------------------------
//  1. Enumerations fortement typees
// -----------------------------------------------------------------------------
//  `enum class` (C++11) est l'equivalent d'un enum C# : portee propre, pas de
//  conversion implicite vers un entier. L'ancien `enum` C se convertit
//  silencieusement en int, ce que MISRA interdit.
//
//  Preciser le type sous-jacent (`: avio::u8`) fixe la taille : indispensable
//  quand l'enum traverse un bus (ARINC 429, CAN) ou une frontiere memoire.
// -----------------------------------------------------------------------------
enum class SensorId : avio::u8 {
    LeftPitot = 0U,
    RightPitot = 1U,
    StandbyPitot = 2U,
    StaticPort = 3U,
    Count = 4U
};

/// Convertit une enumeration en son type sous-jacent, explicitement.
/// (equivalent de std::to_underlying, qui n'arrive qu'en C++23)
template <typename E>
constexpr std::underlying_type_t<E> to_underlying(E value) noexcept {
    static_assert(std::is_enum_v<E>, "to_underlying attend une enumeration");
    return static_cast<std::underlying_type_t<E>>(value);
}

/// Verifie qu'une valeur brute recue de l'exterieur correspond bien a un
/// membre valide. Une enum class NE GARANTIT PAS que la valeur soit valide :
/// static_cast<SensorId>(200) compile et produit une enum "hors domaine".
/// C'est un point de robustesse a tester systematiquement.
bool is_valid(SensorId id) noexcept;

/// Nom lisible, pour les journaux de test et de maintenance.
const char* name_of(SensorId id) noexcept;

// -----------------------------------------------------------------------------
//  2. Arithmetique saturante
// -----------------------------------------------------------------------------
//  Debordement d'un entier SIGNE en C++ = COMPORTEMENT INDEFINI (UB).
//  Ce n'est pas une valeur bizarre : le compilateur a le droit de supposer que
//  cela n'arrive jamais et de supprimer vos tests de garde. En C#, `int`
//  deborde en silence (mode unchecked) ou leve OverflowException (checked) :
//  c'est defini dans les deux cas. En C++, il n'y a rien.
//
//  Debordement d'un entier NON SIGNE = defini (arithmetique modulo 2^N).
//  C'est utile, mais rarement ce que l'on veut pour une grandeur physique.
//
//  En avionique, on choisit explicitement une strategie : saturer, signaler,
//  ou passiver. Le silence n'est pas une option.
// -----------------------------------------------------------------------------

constexpr avio::i16 kI16Min = std::numeric_limits<avio::i16>::min();
constexpr avio::i16 kI16Max = std::numeric_limits<avio::i16>::max();

/// Addition saturante sur 16 bits signes : le resultat est borne, jamais UB.
constexpr avio::i16 saturating_add(avio::i16 lhs, avio::i16 rhs) noexcept {
    // Le calcul est fait dans un type strictement plus large : impossible de
    // deborder pendant l'operation elle-meme.
    const avio::i32 sum = static_cast<avio::i32>(lhs) + static_cast<avio::i32>(rhs);
    if (sum > static_cast<avio::i32>(kI16Max)) {
        return kI16Max;
    }
    if (sum < static_cast<avio::i32>(kI16Min)) {
        return kI16Min;
    }
    return static_cast<avio::i16>(sum);
}

/// Soustraction saturante sur 16 bits signes.
constexpr avio::i16 saturating_sub(avio::i16 lhs, avio::i16 rhs) noexcept {
    const avio::i32 difference = static_cast<avio::i32>(lhs) - static_cast<avio::i32>(rhs);
    if (difference > static_cast<avio::i32>(kI16Max)) {
        return kI16Max;
    }
    if (difference < static_cast<avio::i32>(kI16Min)) {
        return kI16Min;
    }
    return static_cast<avio::i16>(difference);
}

/// Multiplication saturante sur 16 bits signes.
constexpr avio::i16 saturating_mul(avio::i16 lhs, avio::i16 rhs) noexcept {
    const avio::i32 product = static_cast<avio::i32>(lhs) * static_cast<avio::i32>(rhs);
    if (product > static_cast<avio::i32>(kI16Max)) {
        return kI16Max;
    }
    if (product < static_cast<avio::i32>(kI16Min)) {
        return kI16Min;
    }
    return static_cast<avio::i16>(product);
}

/// Addition 32 bits AVEC signalement : renvoie false si le resultat aurait
/// deborde. Variante "signaler" plutot que "saturer" : le choix depend de
/// l'exigence, pas du gout du developpeur.
bool checked_add(avio::i32 lhs, avio::i32 rhs, avio::i32& result) noexcept;

/// Division protegee : renvoie false si le diviseur est nul, ou si le calcul
/// deborde (kI32Min / -1 n'est pas representable).
bool checked_div(avio::i32 numerator, avio::i32 denominator, avio::i32& result) noexcept;

// -----------------------------------------------------------------------------
//  3. Conversions verifiees
// -----------------------------------------------------------------------------

/// Conversion entiere verifiee : `out` n'est ecrit que si la conversion est
/// exacte. Technique de l'aller-retour, doublee d'un controle de signe.
///
/// C'est ce qui a manque a Ariane 5.
template <typename To, typename From>
bool checked_cast(From value, To& out) noexcept {
    static_assert(std::is_integral_v<From>, "checked_cast : source entiere attendue");
    static_assert(std::is_integral_v<To>, "checked_cast : cible entiere attendue");

    const To converted = static_cast<To>(value);
    const bool round_trip_ok = (static_cast<From>(converted) == value);

    bool sign_ok = true;
    if constexpr (std::is_signed_v<From> && !std::is_signed_v<To>) {
        sign_ok = (value >= From{0});
    } else if constexpr (!std::is_signed_v<From> && std::is_signed_v<To>) {
        sign_ok = (converted >= To{0});
    }

    if (round_trip_ok && sign_ok) {
        out = converted;
        return true;
    }
    out = To{0};
    return false;
}

/// Teste l'appartenance a un intervalle ferme [low, high].
template <typename T>
constexpr bool in_range(T value, T low, T high) noexcept {
    return (value >= low) && (value <= high);
}

/// Ramene une valeur dans [low, high]. Equivalent de std::clamp (C++17),
/// reecrit ici pour rester explicite sur le comportement aux bornes.
template <typename T>
constexpr T clamp(T value, T low, T high) noexcept {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

// -----------------------------------------------------------------------------
//  4. Disposition memoire
// -----------------------------------------------------------------------------
//  En C#, la disposition des champs est decidee par le runtime (sauf
//  [StructLayout]). En C++, elle est decidee par l'ABI de la cible et vous
//  pouvez la calculer. Cela compte enormement en embarque : taille des
//  messages, occupation RAM, alignement impose par le materiel (DMA).
// -----------------------------------------------------------------------------

/// Disposition naive : le compilateur insere du bourrage (padding).
struct TrameNaive {
    avio::u8 header;    // 1 octet, puis 3 octets de bourrage
    avio::u32 payload;  // 4 octets
    avio::u8 checksum;  // 1 octet, puis 3 octets de bourrage final
};

/// Meme information, champs ordonnes du plus large au plus etroit.
struct TrameCompacte {
    avio::u32 payload;
    avio::u8 header;
    avio::u8 checksum;
};

}  // namespace mod01

#endif  // MOD01_SAFE_ARITH_HPP
