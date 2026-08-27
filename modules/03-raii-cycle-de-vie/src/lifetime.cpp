#include "mod03/lifetime.hpp"

namespace mod03 {
namespace {

struct LogEntry {
    LifetimeLog::Event event;
    avio::i32 tag;
};

// Stockage statique : aucune allocation dynamique, taille bornee et connue.
LogEntry g_entries[LifetimeLog::kCapacity] = {};
avio::usize g_entry_count = 0U;

// Etat du banc de peripheriques.
bool g_channel_busy[DeviceBank::kChannelCount] = {};
avio::u32 g_total_acquisitions = 0U;
avio::u32 g_total_releases = 0U;

// Etat des interruptions.
bool g_interrupts_enabled = true;
avio::u32 g_nesting = 0U;
avio::u32 g_max_nesting = 0U;

}  // namespace

// -----------------------------------------------------------------------------
//  LifetimeLog
// -----------------------------------------------------------------------------

void LifetimeLog::reset() noexcept {
    g_entry_count = 0U;
}

void LifetimeLog::record(Event event, avio::i32 tag) noexcept {
    if (g_entry_count >= kCapacity) {
        // Saturation plutot que debordement : le journal est un outil
        // d'observation, il ne doit jamais corrompre la memoire.
        return;
    }
    g_entries[g_entry_count].event = event;
    g_entries[g_entry_count].tag = tag;
    g_entry_count += 1U;
}

avio::usize LifetimeLog::count() noexcept {
    return g_entry_count;
}

bool LifetimeLog::entry(avio::usize index, Event& out_event, avio::i32& out_tag) noexcept {
    if (index >= g_entry_count) {
        return false;
    }
    out_event = g_entries[index].event;
    out_tag = g_entries[index].tag;
    return true;
}

avio::u32 LifetimeLog::count_of(Event event) noexcept {
    avio::u32 total = 0U;
    for (avio::usize index = 0U; index < g_entry_count; ++index) {
        if (g_entries[index].event == event) {
            total += 1U;
        }
    }
    return total;
}

bool LifetimeLog::is_balanced() noexcept {
    const avio::u32 creations =
        count_of(Event::Construct) + count_of(Event::Copy) + count_of(Event::Move);
    return creations == count_of(Event::Destroy);
}

const char* LifetimeLog::event_name(Event event) noexcept {
    switch (event) {
        case Event::Construct:
            return "construction";
        case Event::Copy:
            return "copie";
        case Event::Move:
            return "deplacement";
        case Event::CopyAssign:
            return "affectation par copie";
        case Event::MoveAssign:
            return "affectation par deplacement";
        case Event::Destroy:
            return "destruction";
        default:
            return "inconnu";
    }
}

// -----------------------------------------------------------------------------
//  Traced -- la "regle de 5" au complet
// -----------------------------------------------------------------------------

Traced::Traced(avio::i32 tag) noexcept : tag_(tag) {
    LifetimeLog::record(LifetimeLog::Event::Construct, tag_);
}

Traced::Traced(const Traced& other) noexcept : tag_(other.tag_) {
    LifetimeLog::record(LifetimeLog::Event::Copy, tag_);
}

Traced::Traced(Traced&& other) noexcept : tag_(other.tag_) {
    // Apres un deplacement, la source doit rester dans un etat VALIDE mais
    // non specifie. Ici on la neutralise explicitement : c'est plus sur, et
    // cela rend l'etat post-deplacement testable.
    other.tag_ = 0;
    LifetimeLog::record(LifetimeLog::Event::Move, tag_);
}

Traced& Traced::operator=(const Traced& other) noexcept {
    if (this != &other) {
        tag_ = other.tag_;
    }
    LifetimeLog::record(LifetimeLog::Event::CopyAssign, tag_);
    return *this;
}

Traced& Traced::operator=(Traced&& other) noexcept {
    if (this != &other) {
        tag_ = other.tag_;
        other.tag_ = 0;
    }
    LifetimeLog::record(LifetimeLog::Event::MoveAssign, tag_);
    return *this;
}

Traced::~Traced() noexcept {
    LifetimeLog::record(LifetimeLog::Event::Destroy, tag_);
}

// -----------------------------------------------------------------------------
//  DeviceBank
// -----------------------------------------------------------------------------

void DeviceBank::reset() noexcept {
    for (avio::u8 index = 0U; index < kChannelCount; ++index) {
        g_channel_busy[index] = false;
    }
    g_total_acquisitions = 0U;
    g_total_releases = 0U;
}

avio::u8 DeviceBank::acquire() noexcept {
    for (avio::u8 index = 0U; index < kChannelCount; ++index) {
        if (!g_channel_busy[index]) {
            g_channel_busy[index] = true;
            g_total_acquisitions += 1U;
            return static_cast<avio::u8>(index + 1U);  // 1..N ; 0 = invalide
        }
    }
    return 0U;
}

void DeviceBank::release(avio::u8 channel) noexcept {
    if ((channel == 0U) || (channel > kChannelCount)) {
        return;  // robustesse : identifiant invalide ignore
    }
    const avio::u8 index = static_cast<avio::u8>(channel - 1U);
    if (g_channel_busy[index]) {
        g_channel_busy[index] = false;
        g_total_releases += 1U;
    }
}

bool DeviceBank::is_acquired(avio::u8 channel) noexcept {
    if ((channel == 0U) || (channel > kChannelCount)) {
        return false;
    }
    return g_channel_busy[channel - 1U];
}

avio::u8 DeviceBank::acquired_count() noexcept {
    avio::u8 total = 0U;
    for (avio::u8 index = 0U; index < kChannelCount; ++index) {
        if (g_channel_busy[index]) {
            total = static_cast<avio::u8>(total + 1U);
        }
    }
    return total;
}

avio::u32 DeviceBank::total_acquisitions() noexcept {
    return g_total_acquisitions;
}

avio::u32 DeviceBank::total_releases() noexcept {
    return g_total_releases;
}

// -----------------------------------------------------------------------------
//  ChannelHandle
// -----------------------------------------------------------------------------

ChannelHandle::ChannelHandle() noexcept : channel_(DeviceBank::acquire()) {}

ChannelHandle::~ChannelHandle() noexcept {
    release();
}

ChannelHandle::ChannelHandle(ChannelHandle&& other) noexcept : channel_(other.channel_) {
    // Le point crucial du deplacement : la SOURCE doit abandonner la
    // propriete, sinon son destructeur liberera un canal qui ne lui appartient
    // plus (double liberation).
    other.channel_ = 0U;
}

ChannelHandle& ChannelHandle::operator=(ChannelHandle&& other) noexcept {
    if (this != &other) {
        release();  // liberer la ressource actuelle AVANT d'en prendre une autre
        channel_ = other.channel_;
        other.channel_ = 0U;
    }
    return *this;
}

void ChannelHandle::release() noexcept {
    if (channel_ != 0U) {
        DeviceBank::release(channel_);
        channel_ = 0U;
    }
}

// -----------------------------------------------------------------------------
//  InterruptState / CriticalSection
// -----------------------------------------------------------------------------

void InterruptState::reset() noexcept {
    g_interrupts_enabled = true;
    g_nesting = 0U;
    g_max_nesting = 0U;
}

bool InterruptState::enabled() noexcept {
    return g_interrupts_enabled;
}

void InterruptState::disable() noexcept {
    g_interrupts_enabled = false;
}

void InterruptState::enable() noexcept {
    g_interrupts_enabled = true;
}

avio::u32 InterruptState::max_nesting() noexcept {
    return g_max_nesting;
}

void InterruptState::enter() noexcept {
    g_nesting += 1U;
    if (g_nesting > g_max_nesting) {
        g_max_nesting = g_nesting;
    }
    g_interrupts_enabled = false;
}

void InterruptState::leave() noexcept {
    if (g_nesting > 0U) {
        g_nesting -= 1U;
    }
    // Les interruptions ne sont reactivees qu'a la sortie de la section la
    // plus externe : c'est le comportement attendu d'un compteur d'imbrication.
    if (g_nesting == 0U) {
        g_interrupts_enabled = true;
    }
}

CriticalSection::CriticalSection() noexcept {
    InterruptState::enter();
}

CriticalSection::~CriticalSection() noexcept {
    InterruptState::leave();
}

// -----------------------------------------------------------------------------
//  Demonstrations
// -----------------------------------------------------------------------------

avio::i32 multi_exit_processing(avio::i32 value) noexcept {
    const CriticalSection guard;  // interruptions desactivees ici

    if (value < 0) {
        return 0;  // sortie 1 : le destructeur de `guard` s'execute quand meme
    }
    if (value == 0) {
        return 1;  // sortie 2
    }
    return 2;  // sortie 3
}

void demonstrate_destruction_order() noexcept {
    const Traced first(1);
    const Traced second(2);
    const Traced third(3);
    // Destruction a la sortie de portee : 3, puis 2, puis 1.
    // Ordre INVERSE de la construction, garanti par la norme.
    avio::unused(first, second, third);
}

}  // namespace mod03
