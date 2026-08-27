// =============================================================================
//  Version CONFORME au standard de codage du projet.
//
//  Chaque choix visible ici correspond a une regle du standard, listee dans le
//  README du module. Ce n'est pas du style : c'est de la VERIFIABILITE.
// =============================================================================
#include "mod13/bcd.hpp"

namespace mod13 {

/// @satisfies LLR-BCD-010
/// @satisfies LLR-BCD-011
Result<avio::u32> bcd_to_binary(avio::u16 bcd_word) noexcept {
    // R-04 : toute variable est initialisee a sa declaration.
    avio::u32 valeur = 0U;
    avio::u32 multiplicateur = 1U;
    bool valide = true;

    // R-11 : boucle a bornes CONNUES et constantes -> WCET calculable,
    //        analyse de couverture triviale.
    for (avio::u32 rang = 0U; rang < kBcdDigitCount; ++rang) {
        const avio::u32 decalage = rang * kBcdDigitBits;
        const avio::u32 chiffre = (static_cast<avio::u32>(bcd_word) >> decalage) & kBcdDigitMask;

        // R-02 : aucun nombre magique. `kBcdMaxDigit` porte son sens.
        if (chiffre > kBcdMaxDigit) {
            valide = false;
            // R-12 : pas de `break` cachant une sortie anticipee. On memorise
            //        l'anomalie et on laisse la boucle se terminer : le nombre
            //        d'iterations reste CONSTANT, donc le WCET aussi.
        } else {
            valeur += chiffre * multiplicateur;
        }
        multiplicateur *= 10U;
    }

    // R-13 : point de sortie UNIQUE. Un seul endroit ou poser un point d'arret,
    //        un seul endroit ou verifier la post-condition.
    return valide ? Result<avio::u32>::ok(valeur)
                  : Result<avio::u32>::error(Status::InvalidArgument);
}

/// @satisfies LLR-BCD-020
Result<avio::u16> binary_to_bcd(avio::u32 value) noexcept {
    Result<avio::u16> resultat = Result<avio::u16>::error(Status::OutOfRange);

    if (value <= kBcdMaxValue) {
        avio::u32 mot = 0U;
        avio::u32 reste = value;

        for (avio::u32 rang = 0U; rang < kBcdDigitCount; ++rang) {
            const avio::u32 chiffre = reste % 10U;
            reste /= 10U;
            mot |= (chiffre << (rang * kBcdDigitBits));
        }
        resultat = Result<avio::u16>::ok(static_cast<avio::u16>(mot));
    }

    return resultat;
}

}  // namespace mod13
