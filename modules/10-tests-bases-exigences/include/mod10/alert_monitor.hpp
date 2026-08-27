// =============================================================================
//  Module 10 -- ALERT-MON : surveillance de seuil avec hysteresis et anti-rebond.
//
//  Support du module sur la CONCEPTION DES TESTS. Une machine a etats est le
//  meilleur terrain d'apprentissage : la couverture des ETATS, des TRANSITIONS
//  et des SEQUENCES s'y raisonne concretement.
//
//  Voir requirements/srd.md et requirements/sdd.md.
// =============================================================================
#ifndef MOD10_ALERT_MONITOR_HPP
#define MOD10_ALERT_MONITOR_HPP

#include <avio/types.hpp>

namespace mod10 {

/// Etat de l'alerte. Les etats transitoires sont OBSERVABLES (decision DA-01) :
/// cela rend la machine a etats testable sans inspecter ses membres prives.
enum class AlertState : avio::u8 {
    Inactive = 0U,  ///< pas d'alerte
    Pending = 1U,   ///< seuil depasse, confirmation en cours
    Active = 2U,    ///< alerte confirmee
    Clearing = 3U   ///< retombee en cours de confirmation
};

const char* state_name(AlertState state) noexcept;

/// Parametres de surveillance.
struct AlertConfig {
    avio::f32 raise_threshold = 0.0F;  ///< seuil de montee
    avio::f32 clear_threshold = 0.0F;  ///< seuil de retombee (< raise_threshold)
    avio::u16 confirm_cycles = 1U;     ///< cycles consecutifs pour lever l'alerte
    avio::u16 clear_cycles = 1U;       ///< cycles consecutifs pour l'effacer
};

class AlertMonitor {
public:
    /// Construit un moniteur inactif.
    /// @satisfies LLR-ALERT-010
    /// @satisfies LLR-ALERT-011
    /// @return false si la configuration est invalide (voir HLR-ALERT-004)
    static bool create(const AlertConfig& config, AlertMonitor& out) noexcept;

    /// Traite un echantillon et renvoie le nouvel etat.
    /// @satisfies LLR-ALERT-020
    /// @satisfies LLR-ALERT-021
    /// @satisfies LLR-ALERT-022
    /// @satisfies LLR-ALERT-030
    /// @satisfies LLR-ALERT-031
    /// @satisfies LLR-ALERT-032
    /// @satisfies LLR-ALERT-040
    /// @satisfies LLR-ALERT-050
    AlertState update(avio::f32 sample) noexcept;

    AlertState state() const noexcept { return state_; }

    /// Vrai si l'alerte est PRESENTEE a l'equipage.
    ///
    /// Elle l'est dans l'etat Active, mais AUSSI dans l'etat Clearing : tant
    /// que la retombee n'est pas confirmee, l'alerte reste affichee. Sans
    /// cela, `clear_cycles` ne servirait a rien -- l'alerte disparaitrait des
    /// le premier echantillon sous le seuil, et l'anti-rebond ne jouerait que
    /// dans un sens.
    /// @satisfies LLR-ALERT-033
    bool is_raised() const noexcept {
        return (state_ == AlertState::Active) || (state_ == AlertState::Clearing);
    }

    /// Progression de la confirmation en cours (0 si aucune).
    avio::u16 confirm_progress() const noexcept { return progress_; }

    /// Nombre total d'activations depuis l'initialisation.
    /// @satisfies LLR-ALERT-040
    avio::u32 activation_count() const noexcept { return activations_; }

    /// Nombre d'echantillons non finis rejetes.
    /// @satisfies LLR-ALERT-051
    avio::u32 rejected_samples() const noexcept { return rejected_; }

    const AlertConfig& config() const noexcept { return config_; }

private:
    AlertConfig config_{};
    AlertState state_ = AlertState::Inactive;
    avio::u16 progress_ = 0U;
    avio::u32 activations_ = 0U;
    avio::u32 rejected_ = 0U;
};

}  // namespace mod10

#endif  // MOD10_ALERT_MONITOR_HPP
