// =============================================================================
//  ATTENTION -- CE FICHIER VIOLE VOLONTAIREMENT LE STANDARD DE CODAGE.
//
//  Il n'existe QUE pour la pedagogie du module 13. Il est compile dans une
//  cible separee, avec clang-tidy desactive et les avertissements relaches
//  (voir CMakeLists.txt du module).
//
//  Ce traitement -- isoler, documenter, ne pas polluer le build principal --
//  est exactement celui que l'on reserve en projet reel au code HERITE ou
//  TIERS que l'on ne peut pas modifier.
//
//  La fonction ci-dessous est FONCTIONNELLEMENT CORRECTE : elle passe les
//  memes tests que la version conforme. C'est tout l'interet de l'exercice.
//  Un standard de codage ne parle pas de correction fonctionnelle ; il parle
//  de VERIFIABILITE, de LISIBILITE et de PREVISIBILITE.
//
//  Les douze violations sont numerotees V01 a V12 et corrigees, une par une,
//  dans le README du module.
// =============================================================================
#include "mod13/bcd.hpp"

#include <cstdlib>

// V01 -- directive `using namespace` en portee de fichier.
//        Importe des centaines de noms, cree des ambiguites de resolution de
//        surcharge, et rend impossible de savoir d'ou vient un identifiant.
using namespace avio;

// V02 -- macro de type fonction, sans parentheses protectrices.
//        DIGIT(x, 1+1) se developpe en (((x) >> 1+1*4) & 0x0F) : faux.
//        Une macro n'a pas de type, pas de portee, et n'apparait pas dans le
//        debogueur. `constexpr` fait tout cela mieux.
#define DIGIT(w, i) (((w) >> (i)*4) & 0x0F)

// V03 -- variable globale mutable.
//        Cree un couplage de donnees invisible (module 12), rend la fonction
//        non reentrante, et son etat depend de l'historique des appels.
static u32 g_dernier_resultat = 0U;

namespace mod13_nonconforming {

// V04 -- recursion.
//        Profondeur de pile non triviale a borner (module 08).
static u32 puissance_dix(u32 exposant) {
    if (exposant == 0) {
        return 1;
    }
    return 10 * puissance_dix(exposant - 1);  // V04
}

u32 bcd_to_binary(u16 bcd_word) noexcept {
    // V05 -- variables non initialisees a la declaration.
    u32 resultat;
    int i;

    resultat = 0;

    // V06 -- nombre magique : 4 n'est explique nulle part.
    for (i = 0; i < 4; i++) {
        // V07 -- conversion de type a la maniere du C, non verifiable et
        //        capable de supprimer un `const` sans que rien ne le signale.
        u32 chiffre = (u32)DIGIT((u32)bcd_word, i);

        // V08 -- nombre magique : 9 n'est explique nulle part.
        if (chiffre > 9) {
            // V09 -- point de sortie multiple ET valeur d'erreur "magique"
            //        indistinguable d'un resultat valide par le type.
            return 0xFFFFFFFF;
        }

        resultat += chiffre * puissance_dix((u32)i);
    }

    // V10 -- effet de bord sur une variable globale.
    g_dernier_resultat = resultat;

    // V11 -- operateur virgule : deux effets de bord dans une expression,
    //        ordre d'evaluation source de confusion.
    return (i = 0), resultat;
}

// V12 -- fonction jamais appelee : CODE MORT.
//        En DO-178C, c'est un constat de non-conformite (paragraphe 6.4.4.3) :
//        il faut soit la supprimer, soit la tracer a une exigence.
static u32 conversion_inutilisee(u16 mot) {
    return (u32)mot * 2U;
}

}  // namespace mod13_nonconforming
