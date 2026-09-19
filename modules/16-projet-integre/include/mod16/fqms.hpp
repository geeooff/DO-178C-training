// =============================================================================
//  PROJET INTEGRE -- FQMS : Fuel Quantity Management System.
//
//  Systeme de gestion de la quantite de carburant d'un bireacteur court/moyen
//  courrier. Trois reservoirs : aile gauche, caisson central, aile droite.
//
//  Ce composant rassemble TOUT ce que la formation a couvert :
//
//    module 01  types de largeur fixe, arithmetique bornee
//    module 02  const-correctness, Span, pas d'arithmetique de pointeur
//    module 03  RAII, pas de ressource a liberer ici (regle de 0)
//    module 04  types forts (Mass), invariants de classe, fabriques validantes
//    module 05  hierarchies plates -- aucune fonction virtuelle ici
//    module 06  templates et constantes constexpr
//    module 07  Result<T> et Status : aucune exception
//    module 08  aucune allocation dynamique, tout est statique
//    module 09  exigences HLR/LLR tracees, annotations @satisfies
//    module 10  AlertMonitor reutilise : hysteresis et anti-rebond
//    module 11  decisions extraites en fonctions pures, couvrables en MC/DC
//    module 12  couplage explicite, dependances par parametre
//    module 13  standard de codage applique
//    module 14  identite et integrite du logiciel
//    module 15  arithmetique entiere deterministe, cycle borne
//
//  Voir requirements/srd.md et requirements/sdd.md pour les exigences.
//
//  NIVEAU : DAL B (une indication de quantite de carburant erronee peut
//  conduire a une panne seche -- vol Air Canada 143, module 04).
// =============================================================================
#ifndef MOD16_FQMS_HPP
#define MOD16_FQMS_HPP

#include <avio/span.hpp>
#include <avio/types.hpp>

#include "mod04/units.hpp"
#include "mod07/result.hpp"
#include "mod10/alert_monitor.hpp"

namespace mod16 {

using mod04::Mass;
using mod07::Result;
using mod07::Status;

// -----------------------------------------------------------------------------
//  1. Identification des reservoirs
// -----------------------------------------------------------------------------
enum class TankId : avio::u8 {
    Left = 0U,    ///< aile gauche
    Center = 1U,  ///< caisson central
    Right = 2U,   ///< aile droite
    Count = 3U
};

const char* tank_name(TankId tank) noexcept;

constexpr avio::usize kTankCount = 3U;

// -----------------------------------------------------------------------------
//  2. Jauge de reservoir
// -----------------------------------------------------------------------------
//  Frontiere materielle : convertit la mesure brute du convertisseur
//  analogique-numerique en masse de carburant.
// -----------------------------------------------------------------------------

constexpr avio::i32 kRawMin = 0;
constexpr avio::i32 kRawMax = 4095;

class TankGauge {
public:
    /// Construit une jauge.
    /// @satisfies LLR-FQMS-010
    /// @return false si la capacite est nulle
    static bool create(Mass capacity, TankGauge& out) noexcept;

    /// Ecrit la valeur brute lue sur le convertisseur (banc de test).
    void set_raw(avio::i32 raw) noexcept { raw_ = raw; }

    /// Convertit la mesure brute en masse.
    /// @satisfies LLR-FQMS-011
    /// @satisfies LLR-FQMS-012
    /// @return Status::OutOfRange si la mesure sort de [0 ; 4095]
    Result<Mass> read() const noexcept;

    constexpr Mass capacity() const noexcept { return capacity_; }
    constexpr avio::i32 raw() const noexcept { return raw_; }

private:
    Mass capacity_{};
    avio::i32 raw_ = 0;
};

// -----------------------------------------------------------------------------
//  3. Configuration du systeme
// -----------------------------------------------------------------------------
struct FuelSystemConfig {
    Mass tank_capacity[kTankCount] = {};

    /// Seuil d'alerte bas niveau, sur la quantite TOTALE.
    Mass low_fuel_threshold{};
    /// Hysteresis de l'alerte bas niveau.
    Mass low_fuel_hysteresis{};

    /// Ecart maximal admissible entre les reservoirs d'aile.
    Mass imbalance_threshold{};
    /// Hysteresis de l'alerte de desequilibre.
    Mass imbalance_hysteresis{};

    /// Cycles consecutifs de confirmation et de retombee des alertes.
    avio::u16 confirm_cycles = 5U;
    avio::u16 clear_cycles = 5U;
};

/// Configuration de reference du programme (A320-like).
/// @satisfies LLR-FQMS-001
FuelSystemConfig default_config() noexcept;

// -----------------------------------------------------------------------------
//  4. Compte rendu de cycle
// -----------------------------------------------------------------------------
struct CycleReport {
    /// Masse par reservoir. Vaut 0 pour un reservoir en panne.
    Mass tank_quantity[kTankCount] = {};

    /// Somme des reservoirs VALIDES.
    Mass total{};

    /// Ecart entre les reservoirs d'aile (valeur absolue).
    /// Nul si l'une des deux jauges d'aile est en panne.
    Mass wing_imbalance{};

    /// Panne de jauge, par reservoir.
    bool sensor_fault[kTankCount] = {};

    /// Nombre de jauges valides sur ce cycle.
    avio::u8 valid_tank_count = 0U;

    /// Alerte bas niveau confirmee.
    bool low_fuel_alert = false;

    /// Alerte de desequilibre confirmee.
    bool imbalance_alert = false;

    /// Ok            : les trois jauges sont valides
    /// NotReady      : une ou deux jauges en panne, quantite DEGRADEE
    /// HardwareFault : les trois jauges en panne, aucune quantite disponible
    Status status = Status::NotReady;
};

// -----------------------------------------------------------------------------
//  5. Decisions extraites (couvrables en MC/DC -- module 11)
// -----------------------------------------------------------------------------

/// Statut global du systeme en fonction du nombre de jauges valides.
/// @satisfies LLR-FQMS-030
Status system_status(avio::u8 valid_tank_count) noexcept;

/// Le desequilibre d'aile est-il evaluable ?
/// Il faut que les DEUX jauges d'aile soient valides.
/// @satisfies LLR-FQMS-041
bool imbalance_is_measurable(bool left_valid, bool right_valid) noexcept;

/// L'alerte bas niveau est-elle evaluable ?
/// Il faut que les TROIS jauges soient valides : une quantite partielle
/// declencherait une alerte bas niveau injustifiee.
/// @satisfies LLR-FQMS-051
bool low_fuel_is_measurable(bool left_valid, bool center_valid, bool right_valid) noexcept;

// -----------------------------------------------------------------------------
//  6. Le systeme
// -----------------------------------------------------------------------------
class FuelSystem {
public:
    /// Construit le systeme.
    /// @satisfies LLR-FQMS-020
    /// @return false si la configuration est invalide
    static bool create(const FuelSystemConfig& config, FuelSystem& out) noexcept;

    /// Un cycle de traitement.
    ///
    /// Sequence FIXE (couplage de controle, module 12) :
    ///   1. lecture des trois jauges ;
    ///   2. calcul de la quantite totale sur les jauges valides ;
    ///   3. calcul de l'ecart d'aile, si mesurable ;
    ///   4. mise a jour du moniteur de bas niveau ;
    ///   5. mise a jour du moniteur de desequilibre.
    ///
    /// Temps d'execution BORNE et CONSTANT : aucune boucle non bornee, aucune
    /// allocation, aucun appel virtuel (module 15).
    ///
    /// @satisfies LLR-FQMS-021
    /// @satisfies LLR-FQMS-031
    /// @satisfies LLR-FQMS-040
    /// @satisfies LLR-FQMS-042
    /// @satisfies LLR-FQMS-050
    /// @satisfies LLR-FQMS-052
    CycleReport update(const avio::i32 raw_values[kTankCount]) noexcept;

    const TankGauge& gauge(TankId tank) const noexcept;
    const FuelSystemConfig& config() const noexcept { return config_; }

    /// Nombre de cycles traites depuis la creation.
    avio::u32 cycle_count() const noexcept { return cycle_count_; }

    /// Nombre total de pannes de jauge detectees (donnee de maintenance).
    /// @satisfies LLR-FQMS-060
    avio::u32 fault_count(TankId tank) const noexcept;

private:
    FuelSystemConfig config_{};
    TankGauge gauges_[kTankCount] = {};
    mod10::AlertMonitor low_fuel_monitor_{};
    mod10::AlertMonitor imbalance_monitor_{};
    avio::u32 fault_counts_[kTankCount] = {};
    avio::u32 cycle_count_ = 0U;
};

}  // namespace mod16

#endif  // MOD16_FQMS_HPP
