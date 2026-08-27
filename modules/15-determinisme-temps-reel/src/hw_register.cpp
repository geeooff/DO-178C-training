#include "mod15/hw_register.hpp"

namespace mod15 {

/// @satisfies LLR-DET-030
bool wait_for_bit(SimulatedRegister& reg, avio::u32 bit_mask, avio::u32 max_iterations) noexcept {
    if (bit_mask == 0U) {
        return false;  // robustesse : un masque nul ne sera jamais satisfait
    }

    // Boucle a BORNE CONNUE : le WCET est calculable, et le chien de garde ne
    // se declenchera pas. Une attente active `while (...) {}` sans borne est
    // interdite : c'est le mode de defaillance le plus violent qui soit.
    for (avio::u32 iteration = 0U; iteration < max_iterations; ++iteration) {
        if ((reg.read_and_count() & bit_mask) != 0U) {
            return true;
        }
    }
    return false;
}

}  // namespace mod15
