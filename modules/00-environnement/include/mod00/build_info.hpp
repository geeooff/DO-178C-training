// =============================================================================
//  Module 00 -- build_info : identifier PRECISEMENT l'environnement de build.
//
//  Pourquoi une telle unite dans une formation DO-178C ?
//  Parce que la premiere question d'un auditeur est : "avec quoi exactement
//  ce binaire a-t-il ete produit ?". La DO-178C 11.16 (Software Life Cycle
//  Environment Configuration Index, SECI) impose de figer et d'identifier :
//  compilateur ET version, options de compilation, editeur de liens, systeme
//  d'exploitation hote, outils de test. Un changement de version de
//  compilateur invalide potentiellement toute la verification deja produite.
//
//  Cet en-tete illustre aussi la mecanique C++ de base :
//    * les include guards (evitent la double inclusion) ;
//    * la DECLARATION de fonctions, dont la DEFINITION est ailleurs (.cpp) ;
//    * `constexpr` pour les valeurs connues a la compilation.
// =============================================================================
#ifndef MOD00_BUILD_INFO_HPP
#define MOD00_BUILD_INFO_HPP

#include <avio/types.hpp>

namespace mod00 {

/// Description figee de l'environnement de production du binaire.
struct BuildInfo {
    const char* compiler;      ///< "MSVC", "GCC", "Clang"...
    avio::u32 compiler_major;  ///< version majeure
    avio::u32 compiler_minor;  ///< version mineure
    avio::i64 cpp_standard;    ///< valeur de la macro __cplusplus
    avio::u32 pointer_bits;    ///< 32 ou 64
    bool little_endian;        ///< ordre des octets de la machine
};

/// Renvoie l'environnement de production, determine A LA COMPILATION.
BuildInfo current_build() noexcept;

/// Traduit la valeur de __cplusplus en nom de norme lisible.
const char* cpp_standard_name(avio::i64 value) noexcept;

/// Detecte l'ordre des octets. Utile pour tout echange de donnees binaires
/// (bus ARINC 429/664, CAN, memoire partagee entre calculateurs).
bool is_little_endian() noexcept;

}  // namespace mod00

#endif  // MOD00_BUILD_INFO_HPP
