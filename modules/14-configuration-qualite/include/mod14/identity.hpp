// =============================================================================
//  Module 14 -- identite logicielle et integrite du chargement.
//
//  Tout logiciel embarque certifie porte une IDENTITE :
//    * un PART NUMBER, referencé au dossier de certification et sur la plaque
//      constructeur de l'equipement ;
//    * une VERSION ;
//    * une EMPREINTE (CRC) qui permet de verifier, a chaque demarrage, que le
//      binaire charge est bien celui qui a ete verifie.
//
//  Cette derniere verification n'est pas de la coquetterie. Le chargement
//  logiciel d'un equipement en atelier peut echouer partiellement, la memoire
//  Flash peut se degrader, et un technicien peut charger la mauvaise version.
//  La DO-178C traite ce sujet sous l'angle de la GESTION DE CONFIGURATION
//  (section 7) et du controle du produit charge.
//
//  Ce module fournit :
//    * la verification du format d'un part number ;
//    * un CRC-32 (IEEE 802.3), table calculee a la compilation (module 06) ;
//    * la verification d'integrite d'une image chargee ;
//    * la table des DONNEES DE VIE DU LOGICIEL avec leur categorie de
//      controle CC1 / CC2.
// =============================================================================
#ifndef MOD14_IDENTITY_HPP
#define MOD14_IDENTITY_HPP

#include <avio/span.hpp>
#include <avio/types.hpp>

namespace mod14 {

// -----------------------------------------------------------------------------
//  1. CRC-32 (IEEE 802.3), table constexpr
// -----------------------------------------------------------------------------

/// Polynome CRC-32 reflechi.
constexpr avio::u32 kCrc32Polynomial = 0xEDB88320U;

struct Crc32Table {
    avio::u32 values[256];

    constexpr Crc32Table() noexcept : values{} {
        for (avio::u32 byte = 0U; byte < 256U; ++byte) {
            avio::u32 remainder = byte;
            for (avio::u32 bit = 0U; bit < 8U; ++bit) {
                const bool lsb_set = (remainder & 1U) != 0U;
                remainder >>= 1U;
                if (lsb_set) {
                    remainder ^= kCrc32Polynomial;
                }
            }
            values[byte] = remainder;
        }
    }
};

/// Table en memoire morte : aucun code d'initialisation dans le binaire.
inline constexpr Crc32Table kCrc32Table{};

static_assert(kCrc32Table.values[0] == 0x00000000U, "CRC-32 : entree 0 incorrecte");
static_assert(kCrc32Table.values[1] == 0x77073096U, "CRC-32 : entree 1 incorrecte");
static_assert(kCrc32Table.values[255] == 0x2D02EF8DU, "CRC-32 : entree 255 incorrecte");

/// CRC-32 d'un tampon (init 0xFFFFFFFF, xor final 0xFFFFFFFF).
/// @satisfies LLR-CM-010
avio::u32 crc32(avio::Span<const avio::u8> data) noexcept;

// -----------------------------------------------------------------------------
//  2. Identite logicielle
// -----------------------------------------------------------------------------

/// Longueur exacte d'un part number : "PN-1234567-001".
constexpr avio::usize kPartNumberLength = 14U;

struct SoftwareIdentity {
    const char* part_number = nullptr;  ///< "PN-1234567-001"
    avio::u16 version_major = 0U;
    avio::u16 version_minor = 0U;
    avio::u16 version_patch = 0U;
    avio::u32 expected_crc = 0U;  ///< empreinte du binaire verifie
};

/// Verifie le format d'un part number : "PN-" + 7 chiffres + "-" + 3 chiffres.
/// @satisfies LLR-CM-020
bool is_valid_part_number(const char* part_number) noexcept;

/// Verifie qu'une image chargee correspond a son identite declaree.
/// @satisfies LLR-CM-030
/// @return false si le part number est invalide, si l'image est vide, ou si
///         le CRC calcule differe du CRC attendu
bool verify_load(const SoftwareIdentity& identity, avio::Span<const avio::u8> image) noexcept;

// -----------------------------------------------------------------------------
//  3. Categories de controle de configuration (DO-178C, table 7-1)
// -----------------------------------------------------------------------------
//  CC1 et CC2 definissent le NIVEAU DE RIGUEUR applique a chaque donnee de vie
//  du logiciel. Ce n'est pas une classification d'importance : c'est une
//  classification d'EXIGENCES DE PROCESSUS.
//
//    CC1 -- le plus strict. Exige :
//           identification, tracabilite des changements, revue des
//           changements, controle des baselines, archivage, chargement
//           controle, protection contre les modifications non autorisees.
//
//    CC2 -- allege. Exige : identification, tracabilite des changements,
//           protection contre les modifications non autorisees.
//
//  La categorie de chaque donnee depend du NIVEAU DAL. Exemple : les resultats
//  de verification sont CC1 en DAL A et B, CC2 en DAL C et D.
// -----------------------------------------------------------------------------

enum class ControlCategory : avio::u8 { CC1 = 1U, CC2 = 2U };

/// Une donnee de vie du logiciel (DO-178C section 11).
struct LifeCycleData {
    const char* acronym;     ///< "PSAC", "SRD", "SDD"...
    const char* name;        ///< intitule complet
    avio::u8 section;        ///< paragraphe de la section 11
    ControlCategory dal_ab;  ///< categorie en DAL A et B
    ControlCategory dal_cd;  ///< categorie en DAL C et D
};

/// Table des donnees de vie du logiciel du projet.
/// @satisfies LLR-CM-040
avio::Span<const LifeCycleData> life_cycle_data() noexcept;

/// Recherche par acronyme.
/// @satisfies LLR-CM-041
/// @return nullptr si l'acronyme est inconnu
const LifeCycleData* find_life_cycle_data(const char* acronym) noexcept;

/// Categorie applicable a un niveau DAL donne.
/// @satisfies LLR-CM-042
/// @param dal 'A', 'B', 'C', 'D' ou 'E'
/// @return false si l'acronyme est inconnu ou le niveau invalide
bool control_category_for(const char* acronym, char dal, ControlCategory& out) noexcept;

const char* category_name(ControlCategory category) noexcept;

}  // namespace mod14

#endif  // MOD14_IDENTITY_HPP
