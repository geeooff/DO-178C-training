#include "mod12/coupling_trace.hpp"

namespace mod12 {
namespace {

struct DataSample {
    Interface interface = Interface::SupervisorReadsAcquisition;
    avio::f32 value = 0.0F;
};

avio::u32 g_calls[CouplingTrace::kInterfaceCount] = {};
DataSample g_data[CouplingTrace::kMaxDataSamples] = {};
avio::usize g_data_count = 0U;

}  // namespace

const char* interface_name(Interface interface) noexcept {
    switch (interface) {
        case Interface::SupervisorReadsAcquisition:
            return "Supervisor -> Acquisition::read()";
        case Interface::SupervisorPushesFilter:
            return "Supervisor -> Filter::push()";
        case Interface::SupervisorReadsAverage:
            return "Supervisor -> Filter::average()";
        case Interface::SupervisorResetsFilter:
            return "Supervisor -> Filter::reset()";
        case Interface::Count:
        default:
            return "interface inconnue";
    }
}

void CouplingTrace::reset() noexcept {
    for (avio::usize index = 0U; index < kInterfaceCount; ++index) {
        g_calls[index] = 0U;
    }
    g_data_count = 0U;
}

/// @satisfies LLR-CHAIN-040
void CouplingTrace::record_call(Interface interface) noexcept {
    const avio::usize index = static_cast<avio::usize>(interface);
    if (index < kInterfaceCount) {
        g_calls[index] += 1U;
    }
}

void CouplingTrace::record_data(Interface interface, avio::f32 value) noexcept {
    if (g_data_count >= kMaxDataSamples) {
        return;  // saturation : l'outil d'observation ne corrompt jamais rien
    }
    g_data[g_data_count].interface = interface;
    g_data[g_data_count].value = value;
    g_data_count += 1U;
}

avio::u32 CouplingTrace::call_count(Interface interface) noexcept {
    const avio::usize index = static_cast<avio::usize>(interface);
    return (index < kInterfaceCount) ? g_calls[index] : 0U;
}

avio::usize CouplingTrace::data_count() noexcept {
    return g_data_count;
}

bool CouplingTrace::data_at(avio::usize index, Interface& out_interface,
                            avio::f32& out_value) noexcept {
    if (index >= g_data_count) {
        return false;
    }
    out_interface = g_data[index].interface;
    out_value = g_data[index].value;
    return true;
}

/// @satisfies LLR-CHAIN-040
bool CouplingTrace::all_interfaces_exercised() noexcept {
    return unexercised_count() == 0U;
}

avio::usize CouplingTrace::unexercised_count() noexcept {
    avio::usize total = 0U;
    for (avio::usize index = 0U; index < kInterfaceCount; ++index) {
        if (g_calls[index] == 0U) {
            total += 1U;
        }
    }
    return total;
}

}  // namespace mod12
