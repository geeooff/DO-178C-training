// =============================================================================
//  Module 12 -- couplage DONNEES et couplage CONTROLE.
//
//  Objectif DO-178C A-7.8 : "Verification of Software Integration Process"
//  -- l'analyse de couverture structurelle doit confirmer que les tests
//  d'INTEGRATION exercent les couplages de donnees et de controle entre les
//  composants.
//
//  DEUX NOTIONS DISTINCTES (DO-178C, glossaire)
//  --------------------------------------------
//  COUPLAGE DE DONNEES (data coupling) :
//      "The dependence of a software component on data not exclusively under
//       the control of that software component."
//      -> tout ce qui TRANSITE entre composants : parametres, valeurs de
//         retour, variables globales, memoire partagee, messages de bus.
//
//  COUPLAGE DE CONTROLE (control coupling) :
//      "The manner or degree by which one software component influences the
//       execution of another software component."
//      -> QUI appelle QUI, dans quel ORDRE, sous quelle CONDITION.
//
//  POURQUOI C'EST UN OBJECTIF SEPARE
//  ---------------------------------
//  On peut atteindre 100 % de couverture MC/DC sur CHAQUE composant pris
//  isolement, et n'avoir jamais teste leur ASSEMBLAGE. Les defauts
//  d'integration -- ordre d'appel inverse, donnee non initialisee au premier
//  cycle, unite non convertie a la frontiere -- ne se voient qu'a
//  l'integration. C'est la classe de defauts la plus couteuse a corriger.
//
//  Requis en DAL A, B et C. Pas en D ni E.
//
//  LA CHAINE DE TRAITEMENT DE CE MODULE
//  ------------------------------------
//      +---------------+  read()   +----------+  push()   +--------+
//      |  Supervisor   |---------->| Acquisit.|           | Filter |
//      |               |<----------|          |           |        |
//      |               |  f32      +----------+           |        |
//      |               |----------------------------------->       |
//      |               |<-----------------------------------|       |
//      |               |  average() -> f32                  +--------+
//      +---------------+
//
//  CHOIX DE CONCEPTION : le superviseur est GENERIQUE sur ses dependances
//  (template). Trois consequences :
//    * le couplage est EXPLICITE dans la signature du type ;
//    * les tests peuvent substituer des composants instrumentes SANS
//      modifier le code de production -- point capital : le binaire verifie
//      reste le binaire embarque ;
//    * aucune vtable, aucun cout a l'execution (module 06).
// =============================================================================
#ifndef MOD12_CHAIN_HPP
#define MOD12_CHAIN_HPP

#include "mod07/result.hpp"

#include <avio/types.hpp>

namespace mod12 {

using mod07::Result;
using mod07::Status;

// -----------------------------------------------------------------------------
//  Composant 1 : ACQUISITION
// -----------------------------------------------------------------------------
//  Frontiere materielle : convertit une valeur brute de convertisseur
//  analogique-numerique en grandeur physique.
// -----------------------------------------------------------------------------
class Acquisition {
public:
    static constexpr avio::i32 kRawMin = 0;
    static constexpr avio::i32 kRawMax = 4095;
    static constexpr avio::f32 kScaleUnitsPerCount = 0.25F;  // 4095 -> 1023,75 unites

    /// Simule l'ecriture du registre materiel (banc de test uniquement).
    void set_raw(avio::i32 raw) noexcept { raw_ = raw; }

    /// INTERFACE FOURNIE : lit et convertit la mesure courante.
    /// @satisfies LLR-CHAIN-010
    Result<avio::f32> read() noexcept;

    avio::u32 read_count() const noexcept { return read_count_; }
    avio::u32 reject_count() const noexcept { return reject_count_; }

private:
    avio::i32 raw_ = 0;
    avio::u32 read_count_ = 0U;
    avio::u32 reject_count_ = 0U;
};

// -----------------------------------------------------------------------------
//  Composant 2 : FILTRE
// -----------------------------------------------------------------------------
class Filter {
public:
    static constexpr avio::usize kWindow = 4U;

    /// INTERFACE FOURNIE : ajoute un echantillon a la fenetre glissante.
    /// @satisfies LLR-CHAIN-020
    bool push(avio::f32 sample) noexcept;

    /// INTERFACE FOURNIE : moyenne de la fenetre.
    /// @satisfies LLR-CHAIN-021
    /// @return NotReady tant que la fenetre n'est pas pleine
    Result<avio::f32> average() const noexcept;

    /// INTERFACE FOURNIE : vide la fenetre.
    /// @satisfies LLR-CHAIN-022
    void reset() noexcept;

    avio::usize sample_count() const noexcept { return count_; }

private:
    avio::f32 window_[kWindow] = {};
    avio::usize write_index_ = 0U;
    avio::usize count_ = 0U;
};

// -----------------------------------------------------------------------------
//  Composant 3 : SUPERVISEUR
// -----------------------------------------------------------------------------
//  Il ORCHESTRE les deux autres : c'est lui qui porte le couplage de controle.
//  Generique sur ses dependances -> substituables en test sans toucher au
//  code de production.
// -----------------------------------------------------------------------------

/// Etat de sortie d'un cycle de supervision.
struct CycleOutcome {
    Status status = Status::NotReady;
    avio::f32 filtered_value = 0.0F;
    bool alert = false;
};

template <typename AcquisitionT, typename FilterT>
class Supervisor {
public:
    static constexpr avio::f32 kAlertThreshold = 800.0F;
    static constexpr avio::u32 kMaxConsecutiveRejects = 3U;

    Supervisor(AcquisitionT& acquisition, FilterT& filter) noexcept
        : acquisition_(acquisition), filter_(filter) {}

    /// Un cycle de supervision.
    ///
    /// COUPLAGE DE CONTROLE, explicite et lisible :
    ///   1. Supervisor -> Acquisition::read()
    ///   2. si succes  : Supervisor -> Filter::push()
    ///   3.              Supervisor -> Filter::average()
    ///   4. si 3 rejets consecutifs : Supervisor -> Filter::reset()
    ///
    /// L'etape 4 est CONDITIONNELLE : c'est exactement le genre de couplage
    /// que les tests unitaires de chaque composant ne peuvent pas exercer.
    ///
    /// @satisfies LLR-CHAIN-030
    /// @satisfies LLR-CHAIN-031
    /// @satisfies LLR-CHAIN-032
    CycleOutcome cycle() noexcept {
        CycleOutcome outcome;

        // (1) couplage de controle : Supervisor -> Acquisition
        const Result<avio::f32> mesure = acquisition_.read();

        if (mesure.is_error()) {
            consecutive_rejects_ += 1U;
            outcome.status = mesure.status();

            // (4) couplage de controle CONDITIONNEL : Supervisor -> Filter
            if (consecutive_rejects_ >= kMaxConsecutiveRejects) {
                // Trois mesures invalides d'affilee : la fenetre contient des
                // donnees perimees. Les conserver produirait une moyenne
                // trompeuse -- defaut d'integration classique.
                filter_.reset();
                consecutive_rejects_ = 0U;
                flush_count_ += 1U;
            }
            return outcome;
        }

        consecutive_rejects_ = 0U;

        // (2) couplage de DONNEES : la valeur produite par Acquisition est
        //     consommee par Filter. C'est ici que se logent les erreurs
        //     d'unite et d'echelle.
        (void)filter_.push(mesure.value());

        // (3) couplage de controle : Supervisor -> Filter
        const Result<avio::f32> moyenne = filter_.average();
        if (moyenne.is_error()) {
            outcome.status = moyenne.status();
            return outcome;
        }

        outcome.status = Status::Ok;
        outcome.filtered_value = moyenne.value();
        outcome.alert = moyenne.value() > kAlertThreshold;
        return outcome;
    }

    avio::u32 consecutive_rejects() const noexcept { return consecutive_rejects_; }
    avio::u32 flush_count() const noexcept { return flush_count_; }

private:
    AcquisitionT& acquisition_;
    FilterT& filter_;
    avio::u32 consecutive_rejects_ = 0U;
    avio::u32 flush_count_ = 0U;
};

/// Instanciation embarquee. En DO-178C, la liste des instanciations est une
/// donnee d'ARCHITECTURE : elle appartient au SDD (module 06).
using FlightSupervisor = Supervisor<Acquisition, Filter>;

}  // namespace mod12

#endif  // MOD12_CHAIN_HPP
