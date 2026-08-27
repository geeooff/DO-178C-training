// =============================================================================
//  Module 15 -- proprietes de l'arithmetique IEEE-754, demontrees par le code.
//
//  Ces fonctions ne servent a rien en production : elles servent a etablir,
//  par des tests reproductibles, des faits que tout developpeur embarque doit
//  connaitre. Un fait demontre par un test vaut mieux qu'un fait appris par
//  coeur.
// =============================================================================
#ifndef MOD15_FLOAT_FACTS_HPP
#define MOD15_FLOAT_FACTS_HPP

#include <avio/types.hpp>

namespace mod15 {

/// Seuil d'ABSORPTION en simple precision : 2^24 = 16 777 216.
/// Au-dela, l'ecart entre deux flottants consecutifs depasse 1,0, donc
/// `x + 1.0F == x`.
constexpr avio::f32 kSinglePrecisionAbsorption = 16777216.0F;

/// Vrai si `x + increment` est indiscernable de `x` (absorption).
/// @satisfies LLR-DET-010
bool is_absorbed(avio::f32 x, avio::f32 increment) noexcept;

/// Ecart entre `value` et le flottant simple precision immediatement superieur.
/// @satisfies LLR-DET-011
avio::f32 ulp(avio::f32 value) noexcept;

/// Demonstration de la NON-ASSOCIATIVITE : renvoie vrai si
/// (a + b) + c differe de a + (b + c).
/// @satisfies LLR-DET-012
bool addition_is_non_associative(avio::f32 a, avio::f32 b, avio::f32 c) noexcept;

/// Somme de `count` fois `increment`, en simple precision.
/// @satisfies LLR-DET-013
avio::f32 accumulate_float(avio::f32 increment, avio::u32 count) noexcept;

/// Somme de Kahan : compense l'erreur d'arrondi a chaque etape.
///
/// Interet en avionique : quand on DOIT accumuler en flottant, cette
/// technique borne la derive independamment du nombre d'iterations. Cout :
/// trois operations supplementaires par terme, et un code que le compilateur
/// ne doit PAS "optimiser" (d'ou l'interdiction de /fp:fast).
/// @satisfies LLR-DET-014
avio::f32 accumulate_kahan(avio::f32 increment, avio::u32 count) noexcept;

/// Comparaison a tolerance ABSOLUE.
/// @satisfies LLR-DET-020
bool close_absolute(avio::f32 a, avio::f32 b, avio::f32 tolerance) noexcept;

/// Comparaison a tolerance RELATIVE.
///
/// A retenir : la tolerance absolue convient aux grandeurs a domaine borne
/// (une altitude en pieds) ; la tolerance relative convient aux grandeurs
/// couvrant plusieurs ordres de grandeur. Choisir la mauvaise, c'est soit
/// laisser passer une erreur, soit declarer un echec sans raison.
/// @satisfies LLR-DET-021
bool close_relative(avio::f32 a, avio::f32 b, avio::f32 relative_tolerance) noexcept;

}  // namespace mod15

#endif  // MOD15_FLOAT_FACTS_HPP
