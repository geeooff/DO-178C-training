#include "mod07/arinc429.hpp"

namespace mod07 {
namespace {

constexpr avio::u32 kLabelMask = 0x000000FFU;
constexpr avio::u32 kSdiShift = 8U;
constexpr avio::u32 kSdiMask = 0x00000003U;
constexpr avio::u32 kPayloadShift = 10U;
constexpr avio::u32 kPayloadMask = 0x0007FFFFU;  // 19 bits
constexpr avio::u32 kSsmShift = 29U;
constexpr avio::u32 kSsmMask = 0x00000003U;
constexpr avio::u32 kParityBit = 0x80000000U;

constexpr avio::u32 kPayloadSignBit = 0x00040000U;  // bit 19 de la charge utile
constexpr avio::i32 kPayloadSpan = 0x00080000;      // 2^19

/// Etend le signe d'une valeur sur 19 bits vers un entier 32 bits.
avio::i32 sign_extend_19(avio::u32 payload) noexcept {
    if ((payload & kPayloadSignBit) != 0U) {
        return static_cast<avio::i32>(payload) - kPayloadSpan;
    }
    return static_cast<avio::i32>(payload);
}

}  // namespace

bool is_accepted_label(avio::u8 label) noexcept {
    return (label == kLabelAltitude) || (label == kLabelAirspeed) || (label == kLabelHeading);
}

bool has_odd_parity(avio::u32 word) noexcept {
    avio::u32 bits = 0U;
    avio::u32 reste = word;
    while (reste != 0U) {
        bits += (reste & 1U);
        reste >>= 1U;
    }
    return (bits % 2U) == 1U;
}

Result<Arinc429Word> decode(avio::u32 raw_word) noexcept {
    // 1. INTEGRITE. Toujours en premier : inutile d'interpreter des bits dont
    //    on ne sait pas s'ils sont ceux qui ont ete emis.
    if (!has_odd_parity(raw_word)) {
        return Result<Arinc429Word>::error(Status::ChecksumError);
    }

    // 2. ADRESSAGE. Un mot destine a un autre equipement n'est pas une panne :
    //    c'est le fonctionnement normal d'un bus diffuse. On le signale
    //    neanmoins a l'appelant, qui decide de l'ignorer silencieusement.
    const avio::u8 label = static_cast<avio::u8>(raw_word & kLabelMask);
    if (!is_accepted_label(label)) {
        return Result<Arinc429Word>::error(Status::InvalidArgument);
    }

    // 3. ETAT DE LA SOURCE.
    const SignStatus ssm =
        static_cast<SignStatus>(static_cast<avio::u8>((raw_word >> kSsmShift) & kSsmMask));

    switch (ssm) {
        case SignStatus::FailureWarning:
            return Result<Arinc429Word>::error(Status::HardwareFault);
        case SignStatus::NoComputedData:
        case SignStatus::FunctionalTest:
            // NoComputedData : la source ne sait pas calculer la donnee.
            // FunctionalTest : donnee produite pendant un test de l'equipement
            //   source ; techniquement valide, mais elle NE DOIT PAS etre
            //   utilisee en vol. La confondre avec une donnee normale est un
            //   defaut de securite classique.
            // Les deux se traduisent, pour l'utilisateur, par "pas de donnee
            // exploitable maintenant" : un seul statut suffit. Les regrouper
            // evite aussi deux branches identiques (bugprone-branch-clone).
            return Result<Arinc429Word>::error(Status::NotReady);
        case SignStatus::NormalOperation:
            break;
    }

    // 4. EXTRACTION.
    Arinc429Word decoded;
    decoded.label = label;
    decoded.sdi = static_cast<avio::u8>((raw_word >> kSdiShift) & kSdiMask);
    decoded.payload = (raw_word >> kPayloadShift) & kPayloadMask;
    decoded.ssm = ssm;
    decoded.signed_value = sign_extend_19(decoded.payload);

    return Result<Arinc429Word>::ok(decoded);
}

avio::u32 encode(avio::u8 label, avio::u8 sdi, avio::u32 payload, SignStatus ssm) noexcept {
    avio::u32 word = 0U;
    word |= static_cast<avio::u32>(label) & kLabelMask;
    word |= (static_cast<avio::u32>(sdi) & kSdiMask) << kSdiShift;
    word |= (payload & kPayloadMask) << kPayloadShift;
    word |= (static_cast<avio::u32>(ssm) & kSsmMask) << kSsmShift;

    // Le bit 32 est positionne pour obtenir une parite IMPAIRE sur le mot
    // complet. C'est la definition ARINC 429.
    if (!has_odd_parity(word)) {
        word |= kParityBit;
    }
    return word;
}

Result<avio::i32> extract_altitude_feet(avio::u32 raw_word) noexcept {
    // PROPAGATION EXPLICITE. Chaque etape verifie le resultat de la
    // precedente. Aucun chemin cache : le flot de controle est integralement
    // lisible dans le code, ce qui rend l'analyse de couplage de controle
    // (module 12) directe.
    const Result<Arinc429Word> decoded = decode(raw_word);
    if (decoded.is_error()) {
        return Result<avio::i32>::error(decoded.status());
    }

    const Arinc429Word& word = decoded.value();
    if (word.label != kLabelAltitude) {
        return Result<avio::i32>::error(Status::InvalidArgument);
    }

    // Domaine de vol admissible : de -2000 a +60000 pieds (module 04).
    // Une valeur hors domaine signale une donnee corrompue malgre une parite
    // correcte -- cela arrive (erreur double, defaut de l'emetteur).
    if ((word.signed_value < -2000) || (word.signed_value > 60000)) {
        return Result<avio::i32>::error(Status::OutOfRange);
    }

    return Result<avio::i32>::ok(word.signed_value);
}

}  // namespace mod07
