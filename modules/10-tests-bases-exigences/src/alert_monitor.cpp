#include "mod10/alert_monitor.hpp"

#include <cmath>

namespace mod10 {

const char* state_name(AlertState state) noexcept {
    switch (state) {
        case AlertState::Inactive:
            return "Inactive";
        case AlertState::Pending:
            return "Pending";
        case AlertState::Active:
            return "Active";
        case AlertState::Clearing:
            return "Clearing";
    }
    return "Inconnu";  // robustesse : valeur fabriquee par cast
}

/// @satisfies LLR-ALERT-010
/// @satisfies LLR-ALERT-011
bool AlertMonitor::create(const AlertConfig& config, AlertMonitor& out) noexcept {
    if (!std::isfinite(config.raise_threshold) || !std::isfinite(config.clear_threshold)) {
        return false;
    }
    // Hysteresis STRICTE : sans elle, une valeur oscillant autour du seuil
    // ferait battre l'alerte a chaque cycle.
    if (config.clear_threshold >= config.raise_threshold) {
        return false;
    }
    if ((config.confirm_cycles == 0U) || (config.clear_cycles == 0U)) {
        return false;
    }

    out.config_ = config;
    out.state_ = AlertState::Inactive;
    out.progress_ = 0U;
    out.activations_ = 0U;
    out.rejected_ = 0U;
    return true;
}

/// @satisfies LLR-ALERT-020
/// @satisfies LLR-ALERT-021
/// @satisfies LLR-ALERT-022
/// @satisfies LLR-ALERT-030
/// @satisfies LLR-ALERT-031
/// @satisfies LLR-ALERT-032
/// @satisfies LLR-ALERT-040
/// @satisfies LLR-ALERT-050
AlertState AlertMonitor::update(avio::f32 sample) noexcept {
    // LLR-ALERT-050 : un echantillon non fini est IGNORE. Ni l'etat, ni la
    // progression, ni le compteur d'activations ne bougent. Seul le compteur
    // de rejets avance, pour que le silence reste observable (LLR-ALERT-051).
    if (!std::isfinite(sample)) {
        rejected_ += 1U;
        return state_;
    }

    const bool au_dessus = sample > config_.raise_threshold;
    const bool au_dessous = sample < config_.clear_threshold;

    switch (state_) {
        case AlertState::Inactive:
            if (au_dessus) {
                progress_ = 1U;
                if (progress_ >= config_.confirm_cycles) {
                    state_ = AlertState::Active;
                    progress_ = 0U;
                    activations_ += 1U;
                } else {
                    state_ = AlertState::Pending;
                }
            }
            break;

        case AlertState::Pending:
            if (au_dessus) {
                progress_ = static_cast<avio::u16>(progress_ + 1U);
                if (progress_ >= config_.confirm_cycles) {
                    state_ = AlertState::Active;
                    progress_ = 0U;
                    activations_ += 1U;
                }
            } else {
                // LLR-ALERT-022 : la confirmation porte sur des cycles
                // CONSECUTIFS. Un seul cycle sous le seuil annule tout.
                state_ = AlertState::Inactive;
                progress_ = 0U;
            }
            break;

        case AlertState::Active:
            if (au_dessous) {
                progress_ = 1U;
                if (progress_ >= config_.clear_cycles) {
                    state_ = AlertState::Inactive;
                    progress_ = 0U;
                } else {
                    state_ = AlertState::Clearing;
                }
            }
            break;

        case AlertState::Clearing:
            if (au_dessous) {
                progress_ = static_cast<avio::u16>(progress_ + 1U);
                if (progress_ >= config_.clear_cycles) {
                    state_ = AlertState::Inactive;
                    progress_ = 0U;
                }
            } else {
                // LLR-ALERT-032 / LLR-ALERT-040 : le retour a Active depuis
                // Clearing n'est PAS une nouvelle activation. Compter ici
                // gonflerait le compteur de maintenance a chaque oscillation.
                state_ = AlertState::Active;
                progress_ = 0U;
            }
            break;
    }

    return state_;
}

}  // namespace mod10
