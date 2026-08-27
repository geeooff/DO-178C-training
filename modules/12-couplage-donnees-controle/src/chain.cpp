#include "mod12/chain.hpp"

#include <cmath>

namespace mod12 {

/// @satisfies LLR-CHAIN-010
Result<avio::f32> Acquisition::read() noexcept {
    read_count_ += 1U;

    if ((raw_ < kRawMin) || (raw_ > kRawMax)) {
        reject_count_ += 1U;
        return Result<avio::f32>::error(Status::OutOfRange);
    }

    // COUPLAGE DE DONNEES : c'est ICI que l'unite est fixee. Toute erreur
    // d'echelle a cette frontiere se propage silencieusement dans toute la
    // chaine aval -- c'est le scenario Mars Climate Orbiter (module 04).
    const avio::f32 valeur = static_cast<avio::f32>(raw_) * kScaleUnitsPerCount;
    return Result<avio::f32>::ok(valeur);
}

/// @satisfies LLR-CHAIN-020
bool Filter::push(avio::f32 sample) noexcept {
    if (!std::isfinite(sample)) {
        return false;
    }
    window_[write_index_] = sample;
    write_index_ = (write_index_ + 1U) % kWindow;
    if (count_ < kWindow) {
        count_ += 1U;
    }
    return true;
}

/// @satisfies LLR-CHAIN-021
Result<avio::f32> Filter::average() const noexcept {
    // Tant que la fenetre n'est pas pleine, la moyenne serait biaisee : on
    // refuse de produire une valeur plutot que d'en produire une fausse.
    // C'est une decision de conception, pas une commodite.
    if (count_ < kWindow) {
        return Result<avio::f32>::error(Status::NotReady);
    }

    double somme = 0.0;
    for (avio::usize index = 0U; index < kWindow; ++index) {
        somme += static_cast<double>(window_[index]);
    }
    return Result<avio::f32>::ok(static_cast<avio::f32>(somme / static_cast<double>(kWindow)));
}

/// @satisfies LLR-CHAIN-022
void Filter::reset() noexcept {
    for (avio::usize index = 0U; index < kWindow; ++index) {
        window_[index] = 0.0F;
    }
    write_index_ = 0U;
    count_ = 0U;
}

}  // namespace mod12
