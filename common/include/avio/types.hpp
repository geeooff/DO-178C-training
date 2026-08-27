// =============================================================================
//  avio/types.hpp -- types de base du projet.
//
//  REGLE MISRA C++ (et bon sens embarque) : ne jamais utiliser les types
//  fondamentaux `int`, `long`, `short` dans du code applicatif, car leur
//  taille depend de la cible (LP64, LLP64, 16 bits...). On utilise des alias
//  de largeur EXPLICITE, ce qui rend le code portable et l'analyse de
//  debordement possible.
//
//  Rappel pour un developpeur C# : en C#, `int` fait toujours 32 bits, c'est
//  garanti par la CLI. En C++, `int` fait "au moins 16 bits" et la norme ne
//  promet rien de plus. Sur MSVC x64, `long` fait 32 bits ; sur GCC Linux x64,
//  il en fait 64. C'est une source classique de bugs de portage.
// =============================================================================
#ifndef AVIO_TYPES_HPP
#define AVIO_TYPES_HPP

#include <cstddef>
#include <cstdint>

namespace avio {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;

/// Verifications de representation faites A LA COMPILATION.
/// En DO-178C, une hypothese non verifiee est une hypothese fausse : on la
/// transforme donc en contrainte que le compilateur controle.
static_assert(sizeof(f32) == 4U, "f32 doit faire 32 bits (IEEE-754 simple precision)");
static_assert(sizeof(f64) == 8U, "f64 doit faire 64 bits (IEEE-754 double precision)");
static_assert(static_cast<u8>(-1) == 255U, "arithmetique non signee modulo attendue");

/// Marque un parametre volontairement inutilise sans desactiver l'avertissement
/// globalement. Preferable a un commentaire : l'intention est dans le code.
template <typename... T>
constexpr void unused(const T&...) noexcept {}

}  // namespace avio

#endif  // AVIO_TYPES_HPP
