#include "mod15/schedule.hpp"

namespace mod15 {

const char* partition_name(PartitionId partition) noexcept {
    switch (partition) {
        case PartitionId::FlightControl:
            return "CommandesDeVol (DAL A)";
        case PartitionId::FuelManagement:
            return "Carburant (DAL B)";
        case PartitionId::Maintenance:
            return "Maintenance (DAL D)";
        case PartitionId::Display:
            return "Affichage (DAL C)";
        case PartitionId::Count:
        default:
            return "partition inconnue";
    }
}

MajorFrame::MajorFrame(avio::u32 period_us) noexcept : period_us_(period_us) {}

/// @satisfies LLR-SCH-010
bool MajorFrame::add_window(PartitionId partition, avio::u32 offset_us,
                            avio::u32 duration_us) noexcept {
    if (count_ >= kMaxWindows) {
        return false;
    }
    if (duration_us == 0U) {
        return false;  // une fenetre vide n'a pas de sens
    }
    if (partition >= PartitionId::Count) {
        return false;
    }
    // La fenetre doit tenir ENTIEREMENT dans la periode. Le calcul est fait en
    // 64 bits : offset + duration pourrait deborder sur 32 bits.
    const avio::u64 fin = static_cast<avio::u64>(offset_us) + static_cast<avio::u64>(duration_us);
    if (fin > static_cast<avio::u64>(period_us_)) {
        return false;
    }

    // Aucun CHEVAUCHEMENT : c'est la propriete qui garantit le partitionnement
    // temporel. Deux fenetres qui se recouvrent, et la separation entre
    // niveaux DAL disparait.
    for (avio::usize index = 0U; index < count_; ++index) {
        const avio::u64 debut_existant = static_cast<avio::u64>(windows_[index].offset_us);
        const avio::u64 fin_existante =
            debut_existant + static_cast<avio::u64>(windows_[index].duration_us);
        const avio::u64 debut = static_cast<avio::u64>(offset_us);
        if ((debut < fin_existante) && (debut_existant < fin)) {
            return false;
        }
    }

    windows_[count_].partition = partition;
    windows_[count_].offset_us = offset_us;
    windows_[count_].duration_us = duration_us;
    count_ += 1U;
    return true;
}

const Window& MajorFrame::window(avio::usize index) const noexcept {
    return windows_[(index < count_) ? index : 0U];
}

/// @satisfies LLR-SCH-011
avio::u32 MajorFrame::allocated_us() const noexcept {
    avio::u32 total = 0U;
    for (avio::usize index = 0U; index < count_; ++index) {
        total += windows_[index].duration_us;
    }
    return total;
}

/// @satisfies LLR-SCH-011
avio::u32 MajorFrame::utilisation_percent() const noexcept {
    if (period_us_ == 0U) {
        return 0U;
    }
    return (allocated_us() * 100U) / period_us_;
}

avio::u32 MajorFrame::slack_us() const noexcept {
    const avio::u32 alloue = allocated_us();
    return (alloue >= period_us_) ? 0U : (period_us_ - alloue);
}

/// @satisfies LLR-SCH-012
bool MajorFrame::has_margin(avio::u32 minimum_slack_percent) const noexcept {
    if (period_us_ == 0U) {
        return false;
    }
    const avio::u32 marge_percent = (slack_us() * 100U) / period_us_;
    return marge_percent >= minimum_slack_percent;
}

/// @satisfies LLR-SCH-020
FrameResult run_major_frame(const MajorFrame& frame, const avio::u32* execution_us,
                            avio::usize count) noexcept {
    FrameResult resultat;

    if (execution_us == nullptr) {
        return resultat;
    }

    const avio::usize fenetres = (count < frame.window_count()) ? count : frame.window_count();

    for (avio::usize index = 0U; index < fenetres; ++index) {
        const Window& fenetre = frame.window(index);
        const avio::u32 consomme = execution_us[index];

        if (consomme > fenetre.duration_us) {
            // La partition est INTERROMPUE a la fin de sa fenetre : elle ne
            // vole pas de temps aux suivantes. C'est tout l'interet du
            // partitionnement temporel.
            const avio::u32 depassement = consomme - fenetre.duration_us;
            resultat.overrun_count += 1U;
            resultat.deadline_met = false;
            if (depassement > resultat.worst_overrun_us) {
                resultat.worst_overrun_us = depassement;
                resultat.worst_partition = fenetre.partition;
            }
        }
    }

    return resultat;
}

}  // namespace mod15
