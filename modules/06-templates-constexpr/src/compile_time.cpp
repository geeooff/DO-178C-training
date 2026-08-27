#include "mod06/compile_time.hpp"

#include "mod06/ring_buffer.hpp"
#include "mod06/static_polymorphism.hpp"

namespace mod06 {

avio::u8 crc8(avio::Span<const avio::u8> data, avio::u8 initial) noexcept {
    avio::u8 reste = initial;
    for (avio::usize index = 0U; index < data.size(); ++index) {
        const avio::u8 position = static_cast<avio::u8>(reste ^ data[index]);
        reste = kCrc8Table.values[position];
    }
    return reste;
}

// -----------------------------------------------------------------------------
//  Capteurs statiques
// -----------------------------------------------------------------------------

namespace {

avio::f32 map_lineaire(avio::i32 raw, avio::f32 value_min, avio::f32 value_max) noexcept {
    constexpr avio::f32 kSpan = 4095.0F;
    if (raw <= 0) {
        return value_min;
    }
    if (raw >= 4095) {
        return value_max;
    }
    const avio::f32 position = static_cast<avio::f32>(raw) / kSpan;
    return value_min + (position * (value_max - value_min));
}

}  // namespace

avio::f32 StaticPressureSensor::to_engineering_impl(avio::i32 raw) const noexcept {
    return map_lineaire(raw, 0.0F, 1200.0F);
}

avio::f32 StaticTemperatureSensor::to_engineering_impl(avio::i32 raw) const noexcept {
    return map_lineaire(raw, -60.0F, 80.0F);
}

// -----------------------------------------------------------------------------
//  INSTANCIATIONS EXPLICITES
// -----------------------------------------------------------------------------
//  Ces lignes forcent le compilateur a generer le code des instanciations
//  listees, meme si aucun appel ne les utilise dans cette unite de traduction.
//
//  Interet en DO-178C : la liste des instanciations EMBARQUEES devient
//  EXPLICITE et centralisee, donc revisable et tracable dans le document de
//  conception. Sans cela, il faudrait fouiller tout le code pour savoir
//  combien de copies du template se retrouvent dans le binaire -- et donc
//  combien d'instanciations doivent atteindre l'objectif de couverture.
// -----------------------------------------------------------------------------
template class RingBuffer<avio::i32, 4U>;
template class RingBuffer<avio::f32, 8U>;
template class RingBuffer<avio::u8, 16U>;

}  // namespace mod06
