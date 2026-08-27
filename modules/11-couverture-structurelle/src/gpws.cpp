#include "mod11/gpws.hpp"

namespace mod11 {

/// @satisfies LLR-GPWS-010
Mode4aConditions evaluate_conditions(const Mode4aInputs& inputs) noexcept {
    Mode4aConditions conditions;
    conditions.altitude_below_limit = inputs.radio_altitude_ft < kMode4aAltitudeLimitFt;
    conditions.airspeed_below_limit = inputs.airspeed_kt < kMode4aAirspeedLimitKt;
    conditions.gear_not_down = !inputs.gear_down_locked;
    conditions.airborne = !inputs.on_ground;
    return conditions;
}

/// @satisfies LLR-GPWS-020
bool mode4a_decision(bool altitude_below_limit, bool airspeed_below_limit, bool gear_not_down,
                     bool airborne) noexcept {
    // Les quatre conditions sont deja evaluees : le court-circuit de `&&` ne
    // masque plus rien, et l'analyse MC/DC porte sur des valeurs REELLEMENT
    // prises par chaque condition.
    return altitude_below_limit && airspeed_below_limit && gear_not_down && airborne;
}

/// @satisfies LLR-GPWS-021
bool mode4a_alert(const Mode4aInputs& inputs) noexcept {
    const Mode4aConditions conditions = evaluate_conditions(inputs);
    return mode4a_decision(conditions.altitude_below_limit, conditions.airspeed_below_limit,
                           conditions.gear_not_down, conditions.airborne);
}

/// @satisfies LLR-GPWS-030
bool inhibition_decision(bool test_mode, bool approach_config, bool glideslope_captured) noexcept {
    return test_mode || (approach_config && glideslope_captured);
}

/// @satisfies LLR-GPWS-040
bool effective_alert(const Mode4aInputs& inputs, bool test_mode, bool approach_config,
                     bool glideslope_captured) noexcept {
    const bool alert = mode4a_alert(inputs);
    const bool inhibited = inhibition_decision(test_mode, approach_config, glideslope_captured);
    return alert && !inhibited;
}

}  // namespace mod11
