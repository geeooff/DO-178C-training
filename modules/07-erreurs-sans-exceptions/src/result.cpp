#include "mod07/result.hpp"

namespace mod07 {

const char* status_name(Status status) noexcept {
    // Pas de `default:` ici : le compilateur signale alors tout membre oublie
    // si l'enumeration evolue (C4061/C4062 sous MSVC, -Wswitch ailleurs).
    // C'est une aide a la maintenance qui vaut bien un cas de robustesse
    // supplementaire apres le switch.
    switch (status) {
        case Status::Ok:
            return "Ok";
        case Status::InvalidArgument:
            return "ArgumentInvalide";
        case Status::OutOfRange:
            return "HorsDomaine";
        case Status::ChecksumError:
            return "ErreurIntegrite";
        case Status::NotReady:
            return "NonDisponible";
        case Status::HardwareFault:
            return "PanneMaterielle";
        case Status::Timeout:
            return "Echeance";
    }
    // Atteint uniquement si une valeur hors enumeration est fabriquee par un
    // cast. C'est un cas de ROBUSTESSE trace, donc testable et couvert.
    return "StatutInconnu";
}

bool is_fault(Status status) noexcept {
    // NotReady n'est PAS une panne : c'est un etat transitoire normal au
    // demarrage. Cette distinction est une decision de conception, a ecrire
    // dans le SDD -- pas une subtilite d'implementation.
    return (status == Status::ChecksumError) || (status == Status::HardwareFault) ||
           (status == Status::Timeout) || (status == Status::OutOfRange);
}

// -----------------------------------------------------------------------------
//  StatusCounters
// -----------------------------------------------------------------------------

void StatusCounters::reset() noexcept {
    for (avio::usize index = 0U; index < kStatusCount; ++index) {
        counters_[index] = 0U;
    }
}

void StatusCounters::record(Status status) noexcept {
    const avio::usize index = static_cast<avio::usize>(status);
    if (index < kStatusCount) {
        counters_[index] += 1U;
    }
    // Robustesse : un statut fabrique par cast est ignore plutot que d'ecrire
    // hors du tableau.
}

avio::u32 StatusCounters::count(Status status) const noexcept {
    const avio::usize index = static_cast<avio::usize>(status);
    return (index < kStatusCount) ? counters_[index] : 0U;
}

avio::u32 StatusCounters::total_faults() const noexcept {
    avio::u32 total = 0U;
    for (avio::usize index = 0U; index < kStatusCount; ++index) {
        const Status status = static_cast<Status>(index);
        if (is_fault(status)) {
            total += counters_[index];
        }
    }
    return total;
}

Status StatusCounters::dominant_fault() const noexcept {
    Status dominant = Status::Ok;
    avio::u32 maximum = 0U;
    for (avio::usize index = 0U; index < kStatusCount; ++index) {
        const Status status = static_cast<Status>(index);
        if (is_fault(status) && (counters_[index] > maximum)) {
            maximum = counters_[index];
            dominant = status;
        }
    }
    return dominant;
}

}  // namespace mod07
