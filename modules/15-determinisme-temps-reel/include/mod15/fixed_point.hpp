// =============================================================================
//  Module 15 -- arithmetique en VIRGULE FIXE.
//
//  POURQUOI PAS DE FLOTTANT PARTOUT ?
//  ----------------------------------
//  Le flottant IEEE-754 n'est pas "imprecis" : il est parfaitement defini. Le
//  probleme est ailleurs :
//
//   1. La PRECISION DEPEND DE LA MAGNITUDE. A 1,0 l'ecart entre deux flottants
//      simple precision vaut 1,2e-7 ; a 16 777 216 il vaut 2,0. Ajouter 1,0 a
//      16 777 216 ne change RIEN : c'est l'ABSORPTION.
//   2. L'addition n'est PAS ASSOCIATIVE : (a+b)+c peut differer de a+(b+c).
//      Le compilateur n'a donc pas le droit de reordonner... sauf avec
//      /fp:fast, ou il se l'autorise. Deux binaires, deux resultats.
//   3. Le RESULTAT PEUT DIFFERER D'UNE CIBLE A L'AUTRE : registres x87 80 bits
//      contre SSE 32/64 bits, instruction FMA fusionnee ou non, bibliotheque
//      mathematique differente.
//   4. L'ANALYSE D'ERREUR est difficile : borner l'erreur accumulee sur
//      100 000 iterations demande un vrai travail d'analyse numerique.
//
//  LA VIRGULE FIXE repond a ces quatre points :
//    * la resolution est CONSTANTE sur tout le domaine (ici 2^-16) ;
//    * l'arithmetique est ENTIERE, donc associative et exacte tant qu'il n'y a
//      pas de debordement ;
//    * le resultat est IDENTIQUE sur toute cible disposant d'entiers 32/64 bits ;
//    * l'erreur maximale est CALCULABLE a la main.
//
//  Format retenu : Q16.16 sur 32 bits signes.
//    * 16 bits de partie entiere signee -> domaine [-32768 ; +32767]
//    * 16 bits de partie fractionnaire  -> resolution 1/65536 = 1,5e-5
//
//  C'est le format historique des calculateurs avioniques sans unite
//  flottante, et il reste utilise aujourd'hui pour les grandeurs dont le
//  domaine est borne et connu (angles de gouverne, consignes, pourcentages).
// =============================================================================
#ifndef MOD15_FIXED_POINT_HPP
#define MOD15_FIXED_POINT_HPP

#include <avio/types.hpp>

namespace mod15 {

/// Nombre en virgule fixe Q16.16.
class Fixed {
public:
    /// Nombre de bits de la partie fractionnaire.
    static constexpr avio::u32 kFractionBits = 16U;

    /// Representation interne de la valeur 1,0.
    static constexpr avio::i32 kOne = 1 << kFractionBits;  // 65536

    /// Resolution : plus petit increment representable.
    static constexpr avio::f32 kResolution = 1.0F / 65536.0F;

    static constexpr avio::i32 kRawMax = 2147483647;
    static constexpr avio::i32 kRawMin = -2147483647 - 1;

    constexpr Fixed() noexcept : raw_(0) {}

    /// Construit depuis la representation interne (usage avance et tests).
    static constexpr Fixed from_raw(avio::i32 raw) noexcept { return Fixed(raw); }

    /// Construit depuis un entier. Sature si la valeur sort du domaine.
    /// @satisfies LLR-FIX-010
    static Fixed from_int(avio::i32 value) noexcept;

    /// Construit depuis un flottant. Sature ; renvoie 0 pour NaN.
    /// @satisfies LLR-FIX-011
    static Fixed from_float(avio::f32 value) noexcept;

    constexpr avio::i32 raw() const noexcept { return raw_; }

    /// Partie entiere, tronquee vers moins l'infini.
    /// @satisfies LLR-FIX-012
    avio::i32 to_int() const noexcept;

    avio::f32 to_float() const noexcept;

    // --- Comparaisons : EXACTES, contrairement au flottant -------------------
    friend constexpr bool operator==(Fixed lhs, Fixed rhs) noexcept { return lhs.raw_ == rhs.raw_; }
    friend constexpr bool operator!=(Fixed lhs, Fixed rhs) noexcept { return !(lhs == rhs); }
    friend constexpr bool operator<(Fixed lhs, Fixed rhs) noexcept { return lhs.raw_ < rhs.raw_; }
    friend constexpr bool operator>(Fixed lhs, Fixed rhs) noexcept { return rhs < lhs; }
    friend constexpr bool operator<=(Fixed lhs, Fixed rhs) noexcept { return !(rhs < lhs); }
    friend constexpr bool operator>=(Fixed lhs, Fixed rhs) noexcept { return !(lhs < rhs); }

    // --- Arithmetique SATURANTE ----------------------------------------------
    /// @satisfies LLR-FIX-020
    Fixed operator+(Fixed other) const noexcept;
    /// @satisfies LLR-FIX-021
    Fixed operator-(Fixed other) const noexcept;
    /// @satisfies LLR-FIX-022
    Fixed operator*(Fixed other) const noexcept;

    /// Division. Renvoie false si le diviseur est nul.
    /// @satisfies LLR-FIX-023
    bool divide(Fixed divisor, Fixed& out) const noexcept;

    Fixed operator-() const noexcept;

private:
    explicit constexpr Fixed(avio::i32 raw) noexcept : raw_(raw) {}
    avio::i32 raw_;
};

/// Somme de N valeurs identiques, en virgule fixe.
/// Sert a comparer la derive d'accumulation avec le flottant.
/// @satisfies LLR-FIX-030
Fixed accumulate(Fixed increment, avio::u32 count) noexcept;

}  // namespace mod15

#endif  // MOD15_FIXED_POINT_HPP
