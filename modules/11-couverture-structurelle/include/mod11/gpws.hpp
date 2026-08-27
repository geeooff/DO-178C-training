// =============================================================================
//  Module 11 -- GPWS Mode 4A simplifie : "TOO LOW GEAR".
//
//  Le GPWS (Ground Proximity Warning System) alerte l'equipage d'une
//  proximite dangereuse du sol. Le mode 4A se declenche lorsque l'avion vole
//  bas, lentement, en configuration de croisiere (train rentre) : la
//  configuration typique d'une approche ou l'on a oublie de sortir le train.
//
//  C'est un composant DAL B a DAL A selon les programmes -- donc soumis a
//  l'objectif MC/DC (A-7.5).
//
//  CHOIX DE CONCEPTION DETERMINANT
//  -------------------------------
//  La decision est isolee dans une FONCTION PURE de booleens :
//
//      bool mode4a_decision(bool alt_basse, bool vitesse_basse,
//                           bool train_rentre, bool en_vol) noexcept;
//
//  Trois benefices immediats :
//    1. la decision est testable exhaustivement (16 combinaisons) sans avoir a
//       fabriquer des valeurs physiques ;
//    2. les conditions sont evaluees AVANT la decision, donc le COURT-CIRCUIT
//       de `&&` ne masque plus aucune condition -- l'analyse MC/DC devient
//       exacte ;
//    3. la fonction est sans etat, donc reentrante et analysable.
//
//  C'est la conception qu'il faut savoir defendre en revue : "j'ai extrait la
//  decision pour la rendre couvrable".
// =============================================================================
#ifndef MOD11_GPWS_HPP
#define MOD11_GPWS_HPP

#include <avio/types.hpp>

namespace mod11 {

/// Entrees physiques du mode 4A.
struct Mode4aInputs {
    avio::f32 radio_altitude_ft = 0.0F;  ///< hauteur sol mesuree par radioaltimetre
    avio::f32 airspeed_kt = 0.0F;        ///< vitesse air calculee
    bool gear_down_locked = false;       ///< train sorti et verrouille
    bool on_ground = false;              ///< contact sol (capteur de train)
};

/// Conditions elementaires de la decision, evaluees separement.
struct Mode4aConditions {
    bool altitude_below_limit = false;  ///< C1 : hauteur < 500 ft
    bool airspeed_below_limit = false;  ///< C2 : vitesse < 190 kt
    bool gear_not_down = false;         ///< C3 : train non sorti
    bool airborne = false;              ///< C4 : en vol
};

constexpr avio::f32 kMode4aAltitudeLimitFt = 500.0F;
constexpr avio::f32 kMode4aAirspeedLimitKt = 190.0F;

/// Traduit les entrees physiques en conditions booleennes.
/// @satisfies LLR-GPWS-010
Mode4aConditions evaluate_conditions(const Mode4aInputs& inputs) noexcept;

/// LA DECISION : alerte = C1 ET C2 ET C3 ET C4.
/// @satisfies LLR-GPWS-020
bool mode4a_decision(bool altitude_below_limit, bool airspeed_below_limit, bool gear_not_down,
                     bool airborne) noexcept;

/// Enchaine evaluation des conditions et decision.
/// @satisfies LLR-GPWS-021
bool mode4a_alert(const Mode4aInputs& inputs) noexcept;

// -----------------------------------------------------------------------------
//  Inhibition : une decision MIXTE (OU + ET)
// -----------------------------------------------------------------------------
//  L'alerte est inhibee pendant un test au sol, ou lorsque l'avion est
//  volontairement en configuration d'approche stabilisee.
//
//      inhibition = test_mode OU (config_approche ET plan_capture)
//
//  Trois conditions, mais une structure MIXTE : le jeu de tests MC/DC minimal
//  n'a plus rien d'evident. C'est le cas interessant.
// -----------------------------------------------------------------------------

/// @satisfies LLR-GPWS-030
bool inhibition_decision(bool test_mode, bool approach_config, bool glideslope_captured) noexcept;

/// Alerte effective : mode 4A actif ET non inhibee.
/// @satisfies LLR-GPWS-040
bool effective_alert(const Mode4aInputs& inputs, bool test_mode, bool approach_config,
                     bool glideslope_captured) noexcept;

}  // namespace mod11

#endif  // MOD11_GPWS_HPP
