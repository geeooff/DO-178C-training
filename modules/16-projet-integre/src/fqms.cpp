#include "mod16/fqms.hpp"

#include <limits>

namespace mod16 {
namespace {

/// Echantillon "inconnu" transmis a un AlertMonitor lorsque la grandeur n'est
/// pas mesurable. Le moniteur l'ignore et gele son etat -- comportement
/// specifie par LLR-ALERT-050 du module 10, et REUTILISE ici a dessein.
const avio::f32 kUnknownSample = std::numeric_limits<avio::f32>::quiet_NaN();

/// Ecart absolu entre deux masses.
Mass absolute_difference(Mass a, Mass b) noexcept {
    // Mass::operator- est borne a zero (module 04) : on prend donc le maximum
    // des deux differences. Aucune valeur negative ne peut apparaitre.
    return (a > b) ? (a - b) : (b - a);
}

}  // namespace

const char* tank_name(TankId tank) noexcept {
    switch (tank) {
        case TankId::Left:
            return "AileGauche";
        case TankId::Center:
            return "Central";
        case TankId::Right:
            return "AileDroite";
        case TankId::Count:
        default:
            return "Inconnu";
    }
}

// -----------------------------------------------------------------------------
//  TankGauge
// -----------------------------------------------------------------------------

/// @satisfies LLR-FQMS-010
bool TankGauge::create(Mass capacity, TankGauge& out) noexcept {
    const Mass zero;
    if (capacity == zero) {
        return false;  // un reservoir de capacite nulle n'a pas de sens
    }
    out.capacity_ = capacity;
    out.raw_ = kRawMin;
    return true;
}

/// @satisfies LLR-FQMS-011
/// @satisfies LLR-FQMS-012
Result<Mass> TankGauge::read() const noexcept {
    if ((raw_ < kRawMin) || (raw_ > kRawMax)) {
        // Mesure hors du domaine du convertisseur : jauge en panne, ou
        // cablage coupe. On ne produit AUCUNE quantite : une quantite fausse
        // mais plausible serait plus dangereuse que pas de quantite du tout.
        return Result<Mass>::error(Status::OutOfRange);
    }

    // Conversion lineaire, entierement en arithmetique ENTIERE 64 bits :
    // resultat identique sur toute cible, aucune derive, WCET constant
    // (module 15). Le produit maximal vaut 8e6 x 4095 = 3,3e10 : il faut bien
    // 64 bits pendant le calcul.
    const avio::i64 numerateur =
        static_cast<avio::i64>(capacity_.grams()) * static_cast<avio::i64>(raw_ - kRawMin);
    const avio::i64 denominateur = static_cast<avio::i64>(kRawMax - kRawMin);
    const avio::i32 grammes = static_cast<avio::i32>(numerateur / denominateur);

    Mass quantite;
    if (!Mass::from_grams(grammes, quantite)) {
        // Ne peut pas arriver si la capacite respecte le domaine de Mass ;
        // la branche existe neanmoins car elle est TRACEE a une exigence de
        // robustesse, et elle est exercee par un test (module 07 : pas de
        // code defensif non justifie).
        return Result<Mass>::error(Status::OutOfRange);
    }
    return Result<Mass>::ok(quantite);
}

// -----------------------------------------------------------------------------
//  Configuration de reference
// -----------------------------------------------------------------------------

/// @satisfies LLR-FQMS-001
FuelSystemConfig default_config() noexcept {
    FuelSystemConfig config;

    // Capacites typiques d'un biréacteur court/moyen courrier : 18 000 kg.
    (void)Mass::from_kilograms(5000.0F, config.tank_capacity[0]);  // aile gauche
    (void)Mass::from_kilograms(8000.0F, config.tank_capacity[1]);  // central
    (void)Mass::from_kilograms(5000.0F, config.tank_capacity[2]);  // aile droite

    (void)Mass::from_kilograms(1500.0F, config.low_fuel_threshold);
    (void)Mass::from_kilograms(200.0F, config.low_fuel_hysteresis);
    (void)Mass::from_kilograms(500.0F, config.imbalance_threshold);
    (void)Mass::from_kilograms(100.0F, config.imbalance_hysteresis);

    // 5 cycles a 10 Hz = 0,5 s de confirmation : assez pour filtrer le
    // ballottement du carburant en turbulence, assez court pour alerter a temps.
    config.confirm_cycles = 5U;
    config.clear_cycles = 5U;
    return config;
}

// -----------------------------------------------------------------------------
//  Decisions extraites (module 11)
// -----------------------------------------------------------------------------

/// @satisfies LLR-FQMS-030
Status system_status(avio::u8 valid_tank_count) noexcept {
    if (valid_tank_count == 0U) {
        return Status::HardwareFault;
    }
    if (valid_tank_count < static_cast<avio::u8>(kTankCount)) {
        return Status::NotReady;  // quantite DEGRADEE, mais disponible
    }
    return Status::Ok;
}

/// @satisfies LLR-FQMS-041
bool imbalance_is_measurable(bool left_valid, bool right_valid) noexcept {
    return left_valid && right_valid;
}

/// @satisfies LLR-FQMS-051
bool low_fuel_is_measurable(bool left_valid, bool center_valid, bool right_valid) noexcept {
    // Les TROIS jauges doivent etre valides : une quantite partielle
    // declencherait une alerte bas niveau injustifiee, ce qui conduirait
    // l'equipage a se derouter sans raison.
    return left_valid && center_valid && right_valid;
}

// -----------------------------------------------------------------------------
//  FuelSystem
// -----------------------------------------------------------------------------

/// @satisfies LLR-FQMS-020
bool FuelSystem::create(const FuelSystemConfig& config, FuelSystem& out) noexcept {
    const Mass zero;

    // 1. Chaque reservoir doit avoir une capacite non nulle.
    for (avio::usize index = 0U; index < kTankCount; ++index) {
        if (!TankGauge::create(config.tank_capacity[index], out.gauges_[index])) {
            return false;
        }
    }

    // 2. Les seuils d'alerte doivent etre non nuls et coherents.
    if ((config.low_fuel_threshold == zero) || (config.imbalance_threshold == zero)) {
        return false;
    }
    if (config.low_fuel_hysteresis >= config.low_fuel_threshold) {
        return false;
    }
    if (config.imbalance_hysteresis >= config.imbalance_threshold) {
        return false;
    }

    // 3. Les moniteurs d'alerte (module 10) travaillent sur des GRANDEURS
    //    DERIVEES, exprimees en kilogrammes :
    //      bas niveau    : deficit = seuil - total      -> alerte si > 0
    //      desequilibre  : ecart absolu entre les ailes -> alerte si > seuil
    mod10::AlertConfig low_config;
    low_config.raise_threshold = 0.0F;
    low_config.clear_threshold = -config.low_fuel_hysteresis.kilograms();
    low_config.confirm_cycles = config.confirm_cycles;
    low_config.clear_cycles = config.clear_cycles;
    if (!mod10::AlertMonitor::create(low_config, out.low_fuel_monitor_)) {
        return false;
    }

    mod10::AlertConfig imbalance_config;
    imbalance_config.raise_threshold = config.imbalance_threshold.kilograms();
    imbalance_config.clear_threshold =
        config.imbalance_threshold.kilograms() - config.imbalance_hysteresis.kilograms();
    imbalance_config.confirm_cycles = config.confirm_cycles;
    imbalance_config.clear_cycles = config.clear_cycles;
    if (!mod10::AlertMonitor::create(imbalance_config, out.imbalance_monitor_)) {
        return false;
    }

    out.config_ = config;
    for (avio::usize index = 0U; index < kTankCount; ++index) {
        out.fault_counts_[index] = 0U;
    }
    out.cycle_count_ = 0U;
    return true;
}

/// @satisfies LLR-FQMS-021
/// @satisfies LLR-FQMS-031
/// @satisfies LLR-FQMS-040
/// @satisfies LLR-FQMS-042
/// @satisfies LLR-FQMS-050
/// @satisfies LLR-FQMS-052
CycleReport FuelSystem::update(const avio::i32 raw_values[kTankCount]) noexcept {
    CycleReport rapport;

    if (raw_values == nullptr) {
        // Robustesse : aucune mesure fournie. On ne fabrique pas de valeur ;
        // on declare la panne totale. Les moniteurs sont geles.
        for (avio::usize index = 0U; index < kTankCount; ++index) {
            rapport.sensor_fault[index] = true;
        }
        rapport.status = Status::HardwareFault;
        (void)low_fuel_monitor_.update(kUnknownSample);
        (void)imbalance_monitor_.update(kUnknownSample);
        return rapport;
    }

    cycle_count_ += 1U;

    // --- Etape 1 : lecture des trois jauges ----------------------------------
    // Boucle a bornes CONSTANTES : WCET calculable (module 15).
    bool valide[kTankCount] = {};
    for (avio::usize index = 0U; index < kTankCount; ++index) {
        gauges_[index].set_raw(raw_values[index]);
        const Result<Mass> mesure = gauges_[index].read();

        if (mesure.is_ok()) {
            valide[index] = true;
            rapport.tank_quantity[index] = mesure.value();
            rapport.valid_tank_count = static_cast<avio::u8>(rapport.valid_tank_count + 1U);
        } else {
            valide[index] = false;
            rapport.sensor_fault[index] = true;
            fault_counts_[index] += 1U;
        }
    }

    // --- Etape 2 : quantite totale des reservoirs VALIDES ---------------------
    Mass total;
    for (avio::usize index = 0U; index < kTankCount; ++index) {
        if (valide[index]) {
            total = total + rapport.tank_quantity[index];
        }
    }
    rapport.total = total;
    rapport.status = system_status(rapport.valid_tank_count);

    // --- Etape 3 : ecart d'aile ----------------------------------------------
    const bool ecart_mesurable =
        imbalance_is_measurable(valide[static_cast<avio::usize>(TankId::Left)],
                                valide[static_cast<avio::usize>(TankId::Right)]);

    if (ecart_mesurable) {
        rapport.wing_imbalance =
            absolute_difference(rapport.tank_quantity[static_cast<avio::usize>(TankId::Left)],
                                rapport.tank_quantity[static_cast<avio::usize>(TankId::Right)]);
    }

    // --- Etape 4 : moniteur de bas niveau ------------------------------------
    const bool bas_niveau_mesurable =
        low_fuel_is_measurable(valide[static_cast<avio::usize>(TankId::Left)],
                               valide[static_cast<avio::usize>(TankId::Center)],
                               valide[static_cast<avio::usize>(TankId::Right)]);

    // Grandeur derivee : deficit = seuil - total. L'alerte se leve donc quand
    // le deficit devient positif, c'est-a-dire quand le total passe sous le
    // seuil. L'hysteresis du moniteur (seuil de retombee negatif) impose au
    // total de remonter de `low_fuel_hysteresis` avant d'effacer l'alerte.
    const avio::f32 deficit_kg =
        bas_niveau_mesurable ? (config_.low_fuel_threshold.kilograms() - rapport.total.kilograms())
                             : kUnknownSample;
    (void)low_fuel_monitor_.update(deficit_kg);
    rapport.low_fuel_alert = low_fuel_monitor_.is_raised();

    // --- Etape 5 : moniteur de desequilibre ----------------------------------
    const avio::f32 ecart_kg =
        ecart_mesurable ? rapport.wing_imbalance.kilograms() : kUnknownSample;
    (void)imbalance_monitor_.update(ecart_kg);
    rapport.imbalance_alert = imbalance_monitor_.is_raised();

    return rapport;
}

const TankGauge& FuelSystem::gauge(TankId tank) const noexcept {
    const avio::usize index = static_cast<avio::usize>(tank);
    return gauges_[(index < kTankCount) ? index : 0U];
}

/// @satisfies LLR-FQMS-060
avio::u32 FuelSystem::fault_count(TankId tank) const noexcept {
    const avio::usize index = static_cast<avio::usize>(tank);
    return (index < kTankCount) ? fault_counts_[index] : 0U;
}

}  // namespace mod16
