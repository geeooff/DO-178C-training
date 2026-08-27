// =============================================================================
//  Module 09 -- ADC-ALT : calcul d'altitude barometrique.
//
//  Ce composant est le support du module sur les EXIGENCES et la TRACABILITE.
//  Chaque fonction porte une annotation `@satisfies` qui la relie a une ou
//  plusieurs exigences de bas niveau, definies dans requirements/sdd.md.
//
//  Ces annotations ne sont pas decoratives : l'outil tools/trace_check.py les
//  lit pour reconstruire la matrice de tracabilite et detecter :
//    * une exigence sans code       (exigence non implementee) ;
//    * une exigence sans test       (exigence non verifiee) ;
//    * du code sans exigence        (code non trace -> suspicion de code mort
//                                    ou d'exigence manquante) ;
//    * un test sans exigence        (test orphelin).
//
//  C'est la mise en oeuvre concrete de la TRACABILITE BIDIRECTIONNELLE exigee
//  par la DO-178C (objectifs A-3.6, A-4.6, A-5.5, A-7.x).
// =============================================================================
#ifndef MOD09_ALTITUDE_HPP
#define MOD09_ALTITUDE_HPP

#include <avio/types.hpp>

#include "mod07/result.hpp"

namespace mod09 {

using mod07::Result;
using mod07::Status;

// -----------------------------------------------------------------------------
//  Constantes du modele ISA (OACI Doc 7488)
// -----------------------------------------------------------------------------
//  Toute constante physique doit etre TRACEE a une source normative. Un nombre
//  magique dans le code est un constat de revue.

/// Pression standard au niveau de la mer.
constexpr avio::f32 kIsaSeaLevelPressureHpa = 1013.25F;

/// Coefficient de la forme fermee ISA, en pieds.
constexpr avio::f32 kIsaAltitudeCoefficientFt = 145366.45F;

/// Exposant L.R/(g.M) du modele ISA.
constexpr avio::f32 kIsaExponent = 0.190284F;

/// Domaine de pression statique accepte (HLR-ADCALT-002).
constexpr avio::f32 kStaticPressureMinHpa = 100.0F;
constexpr avio::f32 kStaticPressureMaxHpa = 1100.0F;

/// Domaine de calage altimetrique accepte (HLR-ADCALT-005).
constexpr avio::f32 kQnhMinHpa = 948.0F;
constexpr avio::f32 kQnhMaxHpa = 1084.0F;

// -----------------------------------------------------------------------------
//  Interface
// -----------------------------------------------------------------------------

/// Valide une pression statique.
/// @satisfies LLR-ADCALT-010
/// @return Ok, OutOfRange (hors domaine) ou InvalidArgument (non finie)
Status validate_static_pressure(avio::f32 pressure_hpa) noexcept;

/// Valide un calage altimetrique.
/// @satisfies LLR-ADCALT-030
Status validate_qnh(avio::f32 qnh_hpa) noexcept;

/// Calcule l'altitude-pression en pieds selon le modele ISA.
/// @satisfies LLR-ADCALT-020
/// @satisfies LLR-ADCALT-021
/// @satisfies LLR-ADCALT-022
/// @satisfies LLR-ADCALT-023
Result<avio::f32> pressure_altitude_feet(avio::f32 static_pressure_hpa) noexcept;

/// Calcule l'altitude indiquee, corrigee du calage altimetrique.
/// @satisfies LLR-ADCALT-031
/// @satisfies LLR-ADCALT-032
Result<avio::f32> corrected_altitude_feet(avio::f32 static_pressure_hpa,
                                          avio::f32 qnh_hpa) noexcept;

/// Nombre de pieds par hectopascal au voisinage du niveau de la mer.
/// @satisfies LLR-ADCALT-033
avio::f32 feet_per_hpa() noexcept;

}  // namespace mod09

#endif  // MOD09_ALTITUDE_HPP
