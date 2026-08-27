#include "mod02/buffers.hpp"

namespace mod02 {

void swap_values(avio::i32& lhs, avio::i32& rhs) noexcept {
    const avio::i32 temporary = lhs;
    lhs = rhs;
    rhs = temporary;
}

bool increment_if_valid(avio::i32* value) noexcept {
    if (value == nullptr) {
        return false;
    }
    *value += 1;
    return true;
}

bool find_max(avio::Span<const avio::i32> values, avio::i32& out_max) noexcept {
    if (values.empty()) {
        return false;
    }

    avio::i32 maximum = values[0U];
    for (avio::usize index = 1U; index < values.size(); ++index) {
        if (values[index] > maximum) {
            maximum = values[index];
        }
    }
    out_max = maximum;
    return true;
}

avio::u16 checksum16(avio::Span<const avio::u8> data) noexcept {
    // Somme sur 32 bits pour ne jamais deborder pendant l'accumulation, puis
    // repliement des retenues. Meme principe que la somme de controle IP.
    avio::u32 sum = 0U;

    avio::usize index = 0U;
    while ((index + 1U) < data.size()) {
        const avio::u32 word =
            (static_cast<avio::u32>(data[index]) << 8U) | static_cast<avio::u32>(data[index + 1U]);
        sum += word;
        index += 2U;
    }
    if (index < data.size()) {
        // Nombre impair d'octets : le dernier est traite comme octet de poids fort.
        sum += (static_cast<avio::u32>(data[index]) << 8U);
    }

    while ((sum >> 16U) != 0U) {
        sum = (sum & 0xFFFFU) + (sum >> 16U);
    }
    return static_cast<avio::u16>(~sum & 0xFFFFU);
}

avio::usize copy_bounded(avio::Span<const avio::u8> source,
                         avio::Span<avio::u8> destination) noexcept {
    const avio::usize count =
        (source.size() < destination.size()) ? source.size() : destination.size();
    for (avio::usize index = 0U; index < count; ++index) {
        destination[index] = source[index];
    }
    return count;
}

void fill(avio::Span<avio::u8> destination, avio::u8 value) noexcept {
    for (avio::usize index = 0U; index < destination.size(); ++index) {
        destination[index] = value;
    }
}

bool equals(avio::Span<const avio::u8> lhs, avio::Span<const avio::u8> rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (avio::usize index = 0U; index < lhs.size(); ++index) {
        if (lhs[index] != rhs[index]) {
            return false;
        }
    }
    return true;
}

// -----------------------------------------------------------------------------
//  MeasurementLog
// -----------------------------------------------------------------------------

MeasurementLog::MeasurementLog() noexcept
    : storage_{}, write_index_(0U), count_(0U), overflowed_(false) {
    // La liste d'initialisation initialise TOUS les membres. Un membre oublie
    // resterait indetermine : clang-tidy le signale
    // (cppcoreguidelines-pro-type-member-init).
}

void MeasurementLog::push(avio::i32 measurement) noexcept {
    storage_[write_index_] = measurement;
    write_index_ = (write_index_ + 1U) % kCapacity;

    if (count_ < kCapacity) {
        count_ += 1U;
    } else {
        overflowed_ = true;
    }
}

avio::usize MeasurementLog::size() const noexcept {
    return count_;
}

bool MeasurementLog::has_overflowed() const noexcept {
    return overflowed_;
}

bool MeasurementLog::at(avio::usize index, avio::i32& out_value) const noexcept {
    if (index >= count_) {
        return false;
    }
    // Lorsque le journal a boucle, la plus ancienne mesure encore presente se
    // trouve a write_index_ ; sinon le stockage est simplement lineaire.
    const avio::usize oldest = (count_ == kCapacity) ? write_index_ : 0U;
    const avio::usize physical = (oldest + index) % kCapacity;
    out_value = storage_[physical];
    return true;
}

avio::Span<const avio::i32> MeasurementLog::raw_storage() const noexcept {
    return avio::Span<const avio::i32>(storage_, kCapacity);
}

void MeasurementLog::clear() noexcept {
    for (avio::usize index = 0U; index < kCapacity; ++index) {
        storage_[index] = 0;
    }
    write_index_ = 0U;
    count_ = 0U;
    overflowed_ = false;
}

}  // namespace mod02
