// =============================================================================
//  Module 07 -- application : decodage d'un mot ARINC 429.
//
//  L'ARINC 429 est LE bus de donnees de l'avionique civile depuis 1977 :
//  liaison point a point unidirectionnelle, 12,5 ou 100 kbit/s, mots de
//  32 bits. On le trouve sur A320, A350, B737, B787...
//
//  FORMAT D'UN MOT (le bit 1 est le bit de POIDS FAIBLE) :
//
//    bit  32 | 31 30 | 29 ................ 11 | 10  9 | 8 .......... 1
//    --------+-------+------------------------+-------+----------------
//    parite  |  SSM  |      DONNEE (19 bits)  |  SDI  |  LABEL (8 bits)
//
//    LABEL  identifie la nature de la donnee (altitude, cap, vitesse...),
//           traditionnellement note en OCTAL (par exemple 203 = altitude
//           barometrique).
//    SDI    Source/Destination Identifier : distingue plusieurs equipements
//           identiques (capteur gauche / droit).
//    DONNEE charge utile, au format BNR (binaire signe) ou BCD.
//    SSM    Sign/Status Matrix : etat de la donnee (voir SignStatus).
//    PARITE bit 32, PARITE IMPAIRE sur l'ensemble du mot.
//
//  Ce decodeur illustre une gestion d'erreur complete SANS exception : chaque
//  cas d'anomalie est un `Status`, trace a une exigence, testable et couvert.
// =============================================================================
#ifndef MOD07_ARINC429_HPP
#define MOD07_ARINC429_HPP

#include <avio/types.hpp>

#include "mod07/result.hpp"

namespace mod07 {

/// Sign/Status Matrix pour une donnee au format BNR.
enum class SignStatus : avio::u8 {
    FailureWarning = 0U,  ///< l'equipement source est en panne
    NoComputedData = 1U,  ///< la source ne peut pas calculer la donnee
    FunctionalTest = 2U,  ///< donnee produite pendant un test, NON utilisable en vol
    NormalOperation = 3U  ///< donnee valide
};

/// Mot ARINC 429 decode.
struct Arinc429Word {
    avio::u8 label = 0U;     ///< 8 bits
    avio::u8 sdi = 0U;       ///< 2 bits
    avio::u32 payload = 0U;  ///< 19 bits bruts
    SignStatus ssm = SignStatus::FailureWarning;
    avio::i32 signed_value = 0;  ///< charge utile interpretee en complement a deux (19 bits)
};

/// Labels acceptes par ce recepteur. En ARINC 429, un equipement ne traite
/// QUE les labels qui le concernent : tout autre mot est ignore. Cette liste
/// est une donnee de configuration, a tracer dans le document d'interface (ICD).
///
/// NOTE MISRA : les litteraux OCTAUX sont interdits (un zero initial change
/// silencieusement la valeur : 0203 vaut 131, pas 203). Or les labels ARINC
/// sont TOUJOURS notes en octal dans les documents d'interface. On ecrit donc
/// la valeur en decimal, avec l'octal en commentaire. C'est exactement le
/// genre de detail ou une regle de codage evite un defaut reel.
constexpr avio::u8 kLabelAltitude = 131U;  // label 203 (octal) : altitude barometrique
constexpr avio::u8 kLabelAirspeed = 134U;  // label 206 (octal) : vitesse air calculee
constexpr avio::u8 kLabelHeading = 208U;   // label 320 (octal) : cap magnetique

/// Vrai si le label fait partie de ceux traites par ce recepteur.
bool is_accepted_label(avio::u8 label) noexcept;

/// Parite impaire sur 32 bits : vrai si le nombre de bits a 1 est IMPAIR.
bool has_odd_parity(avio::u32 word) noexcept;

/// Decode un mot brut.
///
/// Cas d'erreur, chacun trace a une exigence de robustesse :
///   * parite paire            -> Status::ChecksumError
///   * label non reconnu       -> Status::InvalidArgument
///   * SSM = FailureWarning    -> Status::HardwareFault
///   * SSM = NoComputedData    -> Status::NotReady
///   * SSM = FunctionalTest    -> Status::NotReady (donnee non utilisable en vol)
///
/// Aucune exception, aucune allocation, temps d'execution borne et constant.
Result<Arinc429Word> decode(avio::u32 raw_word) noexcept;

/// Construit un mot brut a partir de ses champs, en calculant la parite.
/// Sert aux tests, et au simulateur d'equipement sur banc.
avio::u32 encode(avio::u8 label, avio::u8 sdi, avio::u32 payload, SignStatus ssm) noexcept;

// -----------------------------------------------------------------------------
//  Chainage d'erreurs sans exception
// -----------------------------------------------------------------------------

/// Extrait une altitude en pieds depuis un mot brut.
///
/// Montre la PROPAGATION explicite d'une erreur : chaque etape verifie, et
/// remonte le statut de l'etape precedente. C'est plus verbeux qu'un
/// `try/catch`, mais le flot de controle est ENTIEREMENT VISIBLE -- ce que
/// demande l'analyse de couplage de controle (module 12).
///
/// Resolution ARINC 429 pour le label 203 : 1 pied par bit de poids faible.
Result<avio::i32> extract_altitude_feet(avio::u32 raw_word) noexcept;

}  // namespace mod07

#endif  // MOD07_ARINC429_HPP
