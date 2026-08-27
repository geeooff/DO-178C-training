// =============================================================================
//  Module 13 -- standards de codage : le MEME algorithme, deux fois.
//
//  Sujet : conversion BCD -> binaire. Le codage DCB (decimal code binaire) est
//  utilise par de nombreux labels ARINC 429 : chaque groupe de 4 bits porte un
//  chiffre decimal de 0 a 9. Un groupe valant 10 a 15 est INVALIDE.
//
//  Exemple : 0x1234 en BCD represente le nombre decimal 1234.
//            0x12A4 est invalide (le groupe 'A' vaut 10).
//
//  Ce module fournit DEUX implementations rigoureusement equivalentes :
//
//    * mod13::bcd_to_binary()            -- CONFORME au standard du projet ;
//    * mod13_nonconforming::bcd_to_binary() -- fonctionnellement correcte,
//      mais violant douze regles du standard.
//
//  Les MEMES cas de test sont appliques aux deux. Elles passent toutes les
//  deux. C'est tout l'interet de l'exercice : un standard de codage ne parle
//  PAS de correction fonctionnelle, il parle de VERIFIABILITE, de LISIBILITE
//  et de PREVISIBILITE.
// =============================================================================
#ifndef MOD13_BCD_HPP
#define MOD13_BCD_HPP

#include "mod07/result.hpp"

#include <avio/types.hpp>

namespace mod13 {

using mod07::Result;
using mod07::Status;

/// Nombre de groupes de 4 bits dans un mot de 16 bits.
constexpr avio::u32 kBcdDigitCount = 4U;

/// Masque d'un groupe BCD.
constexpr avio::u32 kBcdDigitMask = 0x0FU;

/// Largeur d'un groupe BCD, en bits.
constexpr avio::u32 kBcdDigitBits = 4U;

/// Valeur decimale maximale d'un groupe BCD valide.
constexpr avio::u32 kBcdMaxDigit = 9U;

/// Valeur decimale maximale representable sur 4 groupes.
constexpr avio::u32 kBcdMaxValue = 9999U;

/// Convertit un mot BCD 16 bits en valeur binaire.
///
/// @satisfies LLR-BCD-010
/// @satisfies LLR-BCD-011
/// @return Status::InvalidArgument si l'un des quatre groupes vaut 10 a 15
Result<avio::u32> bcd_to_binary(avio::u16 bcd_word) noexcept;

/// Conversion inverse : valeur binaire -> mot BCD.
/// @satisfies LLR-BCD-020
/// @return Status::OutOfRange si la valeur depasse 9999
Result<avio::u16> binary_to_bcd(avio::u32 value) noexcept;

}  // namespace mod13

// -----------------------------------------------------------------------------
//  Version NON CONFORME, isolee dans son propre espace de noms et compilee
//  avec des options relachees (voir CMakeLists.txt du module).
//
//  C'est exactement le traitement que l'on reserve, en projet reel, au code
//  HERITE ou au code TIERS que l'on ne peut pas modifier : on l'isole, on
//  documente les deviations, et on ne laisse pas ses avertissements polluer le
//  reste du build.
// -----------------------------------------------------------------------------
namespace mod13_nonconforming {

/// Meme contrat que mod13::bcd_to_binary, meme resultat.
/// @return 0xFFFFFFFF en cas d'entree invalide (convention differente !)
avio::u32 bcd_to_binary(avio::u16 bcd_word) noexcept;

}  // namespace mod13_nonconforming

#endif  // MOD13_BCD_HPP
