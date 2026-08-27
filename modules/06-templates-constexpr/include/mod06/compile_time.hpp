// =============================================================================
//  Module 06 -- `constexpr` : deplacer le calcul vers la compilation.
//
//  Pourquoi cela compte en avionique :
//    * ZERO cycle a l'execution : le WCET n'en souffre pas ;
//    * ZERO code d'initialisation : la table est dans la ROM, pas construite
//      au demarrage. Il n'y a donc pas de code d'init a verifier ni a couvrir ;
//    * une erreur de calcul devient une ERREUR DE COMPILATION, detectee avant
//      meme la revue de code ;
//    * la valeur est identique sur toutes les cibles : pas de derive.
//
//  Le contre-exemple classique : une table CRC construite au demarrage par une
//  boucle. Ce code d'initialisation est du code embarque comme un autre : il
//  faut le tracer a une exigence, le tester et le couvrir. En `constexpr`, il
//  n'existe tout simplement pas dans le binaire.
// =============================================================================
#ifndef MOD06_COMPILE_TIME_HPP
#define MOD06_COMPILE_TIME_HPP

#include <avio/span.hpp>
#include <avio/types.hpp>

namespace mod06 {

/// Polynome CRC-8/ATM (x^8 + x^2 + x + 1), utilise par de nombreux protocoles
/// de terrain. La valeur vient de la specification : elle doit etre TRACEE.
constexpr avio::u8 kCrc8Polynomial = 0x07U;

/// Table de 256 entrees, calculee integralement par le compilateur.
struct Crc8Table {
    avio::u8 values[256];

    constexpr Crc8Table() noexcept : values{} {
        for (avio::u32 octet = 0U; octet < 256U; ++octet) {
            avio::u8 reste = static_cast<avio::u8>(octet);
            for (avio::u32 bit = 0U; bit < 8U; ++bit) {
                const bool msb_arme = (reste & 0x80U) != 0U;
                reste = static_cast<avio::u8>(reste << 1U);
                if (msb_arme) {
                    reste = static_cast<avio::u8>(reste ^ kCrc8Polynomial);
                }
            }
            values[octet] = reste;
        }
    }
};

/// L'objet lui-meme est `constexpr` : il reside en memoire morte.
inline constexpr Crc8Table kCrc8Table{};

// Quelques valeurs de reference verifiees A LA COMPILATION. Si le polynome ou
// l'algorithme change, le build echoue immediatement : c'est la forme la plus
// precoce et la moins chere de verification.
static_assert(kCrc8Table.values[0] == 0x00U, "CRC-8 : entree 0 incorrecte");
static_assert(kCrc8Table.values[1] == 0x07U, "CRC-8 : entree 1 incorrecte");
static_assert(kCrc8Table.values[255] == 0xF3U, "CRC-8 : entree 255 incorrecte");

/// Calcul de CRC-8 sur un tampon, par table.
avio::u8 crc8(avio::Span<const avio::u8> data, avio::u8 initial = 0x00U) noexcept;

// -----------------------------------------------------------------------------
//  Fonctions `constexpr` generiques
// -----------------------------------------------------------------------------

/// Puissance entiere, evaluable a la compilation comme a l'execution.
/// La MEME fonction sert dans les deux contextes : plus de duplication entre
/// une macro et une fonction, donc plus de risque de divergence.
constexpr avio::u64 ipow(avio::u32 base, avio::u32 exponent) noexcept {
    avio::u64 resultat = 1U;
    for (avio::u32 index = 0U; index < exponent; ++index) {
        resultat *= static_cast<avio::u64>(base);
    }
    return resultat;
}

/// Nombre de bits a 1 (population count).
constexpr avio::u32 popcount(avio::u32 value) noexcept {
    avio::u32 total = 0U;
    avio::u32 reste = value;
    while (reste != 0U) {
        total += (reste & 1U);
        reste >>= 1U;
    }
    return total;
}

/// Parite paire d'un mot de 32 bits : utilisee sur les bus ARINC 429, ou le
/// bit 32 de chaque mot porte la parite impaire.
constexpr bool even_parity(avio::u32 value) noexcept {
    return (popcount(value) % 2U) == 0U;
}

static_assert(ipow(2U, 10U) == 1024U, "ipow incorrect");
static_assert(popcount(0xFFU) == 8U, "popcount incorrect");
static_assert(even_parity(0x03U), "parite paire attendue pour 0x03");

// -----------------------------------------------------------------------------
//  Table de conversion generee a la compilation
// -----------------------------------------------------------------------------

/// Table de linearisation d'une sonde de temperature (16 points).
/// Dans un vrai calculateur, ce genre de table evite un calcul flottant
/// couteux dans la boucle temps reel : on interpole entre deux points.
struct LinearisationTable {
    static constexpr avio::usize kPointCount = 16U;
    avio::i16 values[kPointCount];

    constexpr LinearisationTable() noexcept : values{} {
        // -60 degres au point 0, +80 degres au point 15, en dixiemes de degre.
        for (avio::usize index = 0U; index < kPointCount; ++index) {
            const avio::i32 min_dixiemes = -600;
            const avio::i32 max_dixiemes = 800;
            const avio::i32 etendue = max_dixiemes - min_dixiemes;
            const avio::i32 valeur =
                min_dixiemes + ((etendue * static_cast<avio::i32>(index)) /
                                static_cast<avio::i32>(kPointCount - 1U));
            values[index] = static_cast<avio::i16>(valeur);
        }
    }
};

inline constexpr LinearisationTable kLinearisation{};

static_assert(kLinearisation.values[0] == -600, "premier point incorrect");
static_assert(kLinearisation.values[15] == 800, "dernier point incorrect");

}  // namespace mod06

#endif  // MOD06_COMPILE_TIME_HPP
