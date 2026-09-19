// =============================================================================
//  Module 08 -- analyse de pile et bannissement de la recursion.
//
//  La pile est une ressource FINIE, dimensionnee a la conception (typiquement
//  4 a 64 ko par tache dans un noyau ARINC 653). Son debordement ne produit pas
//  une exception propre : il ecrase la memoire voisine, souvent celle d'une
//  autre tache. C'est la perte de "freedom from interference".
//
//  La DO-178C cite la pile en toutes lettres : le paragraphe 6.3.4.f
//  (objectif A-5.6, "accuracy and consistency") liste le "stack usage" parmi
//  ce que la revue du code source doit examiner. En pratique, tout dossier de
//  certification contient une ANALYSE DE PILE demontrant que l'usage maximal
//  reste sous le budget alloue, avec une marge (souvent 30 %).
//
//  CE QUI REND L'ANALYSE POSSIBLE :
//    * profondeur d'appel BORNEE et connue -> pas de recursion ;
//    * taille de trame CONNUE -> pas de VLA, pas d'alloca ;
//    * pas de pointeurs de fonction non resolus -> le graphe d'appel est
//      statiquement determinable ;
//    * pas d'exceptions -> pas de trames de deroulement imprevues.
//
//  On voit que la plupart des interdits de ce cours convergent vers UN seul
//  objectif : rendre le graphe d'appel STATIQUEMENT ANALYSABLE.
// =============================================================================
#ifndef MOD08_STACK_ANALYSIS_HPP
#define MOD08_STACK_ANALYSIS_HPP

#include <avio/types.hpp>

namespace mod08 {

/// Observateur de profondeur d'appel. Dans un vrai systeme, on mesure la pile
/// par "marquage" (remplissage d'un motif connu au demarrage, relecture apres
/// essai). Ici on compte les niveaux, ce qui suffit a la demonstration.
class CallDepthMonitor {
public:
    static void reset() noexcept;
    static avio::u32 current() noexcept;
    static avio::u32 maximum() noexcept;

    /// Garde RAII (module 03) : incremente a l'entree, decremente a la sortie,
    /// sur TOUS les chemins.
    class Scope {
    public:
        Scope() noexcept;
        ~Scope() noexcept;
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&&) = delete;
        Scope& operator=(Scope&&) = delete;
    };
};

// -----------------------------------------------------------------------------
//  Version ITERATIVE : profondeur d'appel constante
// -----------------------------------------------------------------------------

/// Factorielle, sans recursion. Profondeur de pile : 1 niveau, quel que soit n.
/// @return false si le resultat n'est pas representable sur 64 bits (n > 20)
bool factorial(avio::u32 n, avio::u64& out_result) noexcept;

/// Somme des elements d'un tableau, iterative.
avio::i64 sum_iterative(const avio::i32* values, avio::usize count) noexcept;

/// Recherche dichotomique ITERATIVE dans un tableau trie.
/// La version recursive du meme algorithme aurait une profondeur log2(N) --
/// bornee, certes, mais l'iteratif est preferable : profondeur 1, et pas de
/// justification a rediger.
/// @return false si la valeur est absente
bool binary_search(const avio::i32* sorted_values, avio::usize count, avio::i32 target,
                   avio::usize& out_index) noexcept;

// -----------------------------------------------------------------------------
//  Version RECURSIVE : contre-exemple pedagogique
// -----------------------------------------------------------------------------

/// CONTRE-EXEMPLE. Cette fonction existe UNIQUEMENT pour mesurer la
/// profondeur de pile atteinte et la comparer a la version iterative.
///
/// Elle ne serait acceptable dans un projet certifie qu'a trois conditions,
/// toutes documentees dans le dossier de conception :
///   1. la profondeur maximale est DEMONTREE bornee (ici : n, avec n <= 20) ;
///   2. la taille de trame est connue (lisible dans la carte d'edition de
///      liens ou l'assembleur genere) ;
///   3. le produit profondeur x trame est inscrit au budget de pile.
/// Autant dire que l'on ecrit la version iterative.
bool factorial_recursive(avio::u32 n, avio::u64& out_result) noexcept;

}  // namespace mod08

#endif  // MOD08_STACK_ANALYSIS_HPP
