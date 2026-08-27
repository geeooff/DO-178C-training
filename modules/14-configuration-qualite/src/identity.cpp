#include "mod14/identity.hpp"

#include <cstring>

namespace mod14 {
namespace {

/// Table des donnees de vie du logiciel (DO-178C section 11).
/// Les categories CC1/CC2 sont celles de la table 7-1 de la norme.
constexpr LifeCycleData kLifeCycleData[] = {
    {"PSAC", "Plan for Software Aspects of Certification", 1U, ControlCategory::CC1,
     ControlCategory::CC1},
    {"SDP", "Software Development Plan", 2U, ControlCategory::CC1, ControlCategory::CC2},
    {"SVP", "Software Verification Plan", 3U, ControlCategory::CC1, ControlCategory::CC2},
    {"SCMP", "Software Configuration Management Plan", 4U, ControlCategory::CC1,
     ControlCategory::CC2},
    {"SQAP", "Software Quality Assurance Plan", 5U, ControlCategory::CC1, ControlCategory::CC2},
    {"SRS", "Software Requirements Standards", 6U, ControlCategory::CC1, ControlCategory::CC2},
    {"SDS", "Software Design Standards", 7U, ControlCategory::CC1, ControlCategory::CC2},
    {"SCS", "Software Code Standards", 8U, ControlCategory::CC1, ControlCategory::CC2},
    {"SRD", "Software Requirements Data", 9U, ControlCategory::CC1, ControlCategory::CC1},
    {"SDD", "Design Description", 10U, ControlCategory::CC1, ControlCategory::CC2},
    {"SRC", "Source Code", 11U, ControlCategory::CC1, ControlCategory::CC1},
    {"EOC", "Executable Object Code", 12U, ControlCategory::CC1, ControlCategory::CC1},
    {"SVCP", "Software Verification Cases and Procedures", 13U, ControlCategory::CC1,
     ControlCategory::CC2},
    {"SVR", "Software Verification Results", 14U, ControlCategory::CC2, ControlCategory::CC2},
    {"SECI", "Software Life Cycle Environment Configuration Index", 15U, ControlCategory::CC1,
     ControlCategory::CC1},
    {"SCI", "Software Configuration Index", 16U, ControlCategory::CC1, ControlCategory::CC1},
    {"SCR", "Problem Reports", 17U, ControlCategory::CC2, ControlCategory::CC2},
    {"SCMR", "Software Configuration Management Records", 18U, ControlCategory::CC2,
     ControlCategory::CC2},
    {"SQAR", "Software Quality Assurance Records", 19U, ControlCategory::CC2,
     ControlCategory::CC2},
    {"SAS", "Software Accomplishment Summary", 20U, ControlCategory::CC1, ControlCategory::CC1}};

constexpr avio::usize kLifeCycleDataCount = sizeof(kLifeCycleData) / sizeof(kLifeCycleData[0]);

bool is_digit(char character) noexcept {
    return (character >= '0') && (character <= '9');
}

}  // namespace

// -----------------------------------------------------------------------------
//  CRC-32
// -----------------------------------------------------------------------------

/// @satisfies LLR-CM-010
avio::u32 crc32(avio::Span<const avio::u8> data) noexcept {
    avio::u32 reste = 0xFFFFFFFFU;
    for (avio::usize index = 0U; index < data.size(); ++index) {
        const avio::u32 position = (reste ^ static_cast<avio::u32>(data[index])) & 0xFFU;
        reste = kCrc32Table.values[position] ^ (reste >> 8U);
    }
    return reste ^ 0xFFFFFFFFU;
}

// -----------------------------------------------------------------------------
//  Identite
// -----------------------------------------------------------------------------

/// @satisfies LLR-CM-020
bool is_valid_part_number(const char* part_number) noexcept {
    if (part_number == nullptr) {
        return false;
    }
    if (std::strlen(part_number) != kPartNumberLength) {
        return false;
    }
    // Format impose : "PN-1234567-001"
    //                  0123456789ABCD
    if ((part_number[0] != 'P') || (part_number[1] != 'N') || (part_number[2] != '-')) {
        return false;
    }
    for (avio::usize index = 3U; index < 10U; ++index) {
        if (!is_digit(part_number[index])) {
            return false;
        }
    }
    if (part_number[10] != '-') {
        return false;
    }
    for (avio::usize index = 11U; index < 14U; ++index) {
        if (!is_digit(part_number[index])) {
            return false;
        }
    }
    return true;
}

/// @satisfies LLR-CM-030
bool verify_load(const SoftwareIdentity& identity, avio::Span<const avio::u8> image) noexcept {
    // Ordre de verification SPECIFIE : identite, puis presence, puis
    // integrite. Un ordre non specifie rendrait le comportement dependant de
    // l'implementation en cas d'anomalies multiples (module 09).
    if (!is_valid_part_number(identity.part_number)) {
        return false;
    }
    if (image.empty()) {
        return false;
    }
    return crc32(image) == identity.expected_crc;
}

// -----------------------------------------------------------------------------
//  Donnees de vie du logiciel
// -----------------------------------------------------------------------------

/// @satisfies LLR-CM-040
avio::Span<const LifeCycleData> life_cycle_data() noexcept {
    return avio::Span<const LifeCycleData>(kLifeCycleData, kLifeCycleDataCount);
}

/// @satisfies LLR-CM-041
const LifeCycleData* find_life_cycle_data(const char* acronym) noexcept {
    if (acronym == nullptr) {
        return nullptr;
    }
    for (avio::usize index = 0U; index < kLifeCycleDataCount; ++index) {
        if (std::strcmp(kLifeCycleData[index].acronym, acronym) == 0) {
            return &kLifeCycleData[index];
        }
    }
    return nullptr;
}

/// @satisfies LLR-CM-042
bool control_category_for(const char* acronym, char dal, ControlCategory& out) noexcept {
    const LifeCycleData* donnee = find_life_cycle_data(acronym);
    if (donnee == nullptr) {
        return false;
    }
    if ((dal == 'A') || (dal == 'B')) {
        out = donnee->dal_ab;
        return true;
    }
    if ((dal == 'C') || (dal == 'D')) {
        out = donnee->dal_cd;
        return true;
    }
    // DAL E : aucun objectif DO-178C, donc aucune categorie de controle.
    return false;
}

const char* category_name(ControlCategory category) noexcept {
    switch (category) {
        case ControlCategory::CC1:
            return "CC1";
        case ControlCategory::CC2:
            return "CC2";
    }
    return "inconnue";
}

}  // namespace mod14
