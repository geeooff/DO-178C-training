// =============================================================================
//  Module 12 -- OUTILLAGE DE VERIFICATION (non embarque).
//
//  Ce fichier ne fait PAS partie du logiciel embarque. Il fournit :
//    * un registre des interfaces inter-composants ;
//    * des composants INSTRUMENTES qui enregistrent chaque appel et chaque
//      donnee echangee.
//
//  Le point capital, du point de vue de la certification : le code de
//  PRODUCTION n'est pas modifie. Les composants instrumentes sont substitues
//  au niveau du parametre de template du superviseur. Le binaire embarque
//  reste donc rigoureusement celui qui a ete verifie -- ce qui n'est pas le
//  cas d'une instrumentation par #ifdef.
//
//  A ranger dans les "Software Verification Cases and Procedures" (11.13),
//  pas dans le "Source Code" (11.11).
// =============================================================================
#ifndef MOD12_COUPLING_TRACE_HPP
#define MOD12_COUPLING_TRACE_HPP

#include <avio/types.hpp>

#include "mod12/chain.hpp"

namespace mod12 {

/// Les interfaces inter-composants du sous-systeme.
/// Cette liste est le RESULTAT de l'analyse de couplage de controle : elle est
/// etablie a la lecture de la conception, puis confrontee aux tests.
enum class Interface : avio::u8 {
    SupervisorReadsAcquisition = 0U,  ///< Supervisor -> Acquisition::read()
    SupervisorPushesFilter = 1U,      ///< Supervisor -> Filter::push()
    SupervisorReadsAverage = 2U,      ///< Supervisor -> Filter::average()
    SupervisorResetsFilter = 3U,      ///< Supervisor -> Filter::reset()  (CONDITIONNEL)
    Count = 4U
};

const char* interface_name(Interface interface) noexcept;

/// Enregistreur des appels et des donnees echangees.
class CouplingTrace {
public:
    static constexpr avio::usize kInterfaceCount = 4U;
    static constexpr avio::usize kMaxDataSamples = 64U;

    static void reset() noexcept;

    /// Couplage de CONTROLE : un appel a travers une interface.
    /// @satisfies LLR-CHAIN-040
    static void record_call(Interface interface) noexcept;

    /// Couplage de DONNEES : une valeur transitant d'un composant a l'autre.
    static void record_data(Interface interface, avio::f32 value) noexcept;

    static avio::u32 call_count(Interface interface) noexcept;
    static avio::usize data_count() noexcept;
    static bool data_at(avio::usize index, Interface& out_interface, avio::f32& out_value) noexcept;

    /// Vrai si TOUTES les interfaces declarees ont ete exercees au moins une
    /// fois : c'est la demonstration attendue par l'objectif A-7.8.
    static bool all_interfaces_exercised() noexcept;

    /// Nombre d'interfaces jamais exercees.
    static avio::usize unexercised_count() noexcept;
};

// -----------------------------------------------------------------------------
//  Composants instrumentes
// -----------------------------------------------------------------------------
//  Ils DELEGUENT au composant reel : le comportement teste reste celui du code
//  de production. Ils n'ajoutent que l'observation.
// -----------------------------------------------------------------------------

class TracingAcquisition {
public:
    explicit TracingAcquisition(Acquisition& real) noexcept : real_(real) {}

    void set_raw(avio::i32 raw) noexcept { real_.set_raw(raw); }

    Result<avio::f32> read() noexcept {
        CouplingTrace::record_call(Interface::SupervisorReadsAcquisition);
        const Result<avio::f32> result = real_.read();
        if (result.is_ok()) {
            CouplingTrace::record_data(Interface::SupervisorReadsAcquisition, result.value());
        }
        return result;
    }

private:
    Acquisition& real_;
};

class TracingFilter {
public:
    explicit TracingFilter(Filter& real) noexcept : real_(real) {}

    bool push(avio::f32 sample) noexcept {
        CouplingTrace::record_call(Interface::SupervisorPushesFilter);
        CouplingTrace::record_data(Interface::SupervisorPushesFilter, sample);
        return real_.push(sample);
    }

    Result<avio::f32> average() const noexcept {
        CouplingTrace::record_call(Interface::SupervisorReadsAverage);
        const Result<avio::f32> result = real_.average();
        if (result.is_ok()) {
            CouplingTrace::record_data(Interface::SupervisorReadsAverage, result.value());
        }
        return result;
    }

    void reset() noexcept {
        CouplingTrace::record_call(Interface::SupervisorResetsFilter);
        real_.reset();
    }

    avio::usize sample_count() const noexcept { return real_.sample_count(); }

private:
    Filter& real_;
};

/// Superviseur instrumente : MEME code de production, dependances substituees.
using TracedSupervisor = Supervisor<TracingAcquisition, TracingFilter>;

}  // namespace mod12

#endif  // MOD12_COUPLING_TRACE_HPP
