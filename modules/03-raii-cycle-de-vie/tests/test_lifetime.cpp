#include <avio/types.hpp>
#include <microtest/microtest.hpp>
#include <utility>  // std::move

#include "mod03/lifetime.hpp"

using avio::i32;
using avio::u32;
using avio::u8;
using avio::usize;
using Event = mod03::LifetimeLog::Event;

namespace {

/// Remet tout l'etat global a zero. Chaque cas de test doit partir d'un etat
/// connu : en DO-178C, un test dont le resultat depend de l'ordre d'execution
/// n'est pas repetable, donc n'est pas une preuve.
void reset() noexcept {
    mod03::LifetimeLog::reset();
    mod03::DeviceBank::reset();
    mod03::InterruptState::reset();
}

}  // namespace

// -----------------------------------------------------------------------------
//  Ordre de destruction
// -----------------------------------------------------------------------------
TEST_REQ(CycleDeVie, destruction_en_ordre_inverse, "LLR-M03-001") {
    reset();
    mod03::demonstrate_destruction_order();

    REQUIRE_EQ(mod03::LifetimeLog::count(), usize{6});

    Event event = Event::Construct;
    i32 tag = 0;

    // Constructions : 1, 2, 3
    for (usize index = 0U; index < 3U; ++index) {
        REQUIRE(mod03::LifetimeLog::entry(index, event, tag));
        CHECK_EQ(event, Event::Construct);
        CHECK_EQ(tag, static_cast<i32>(index) + 1);
    }
    // Destructions : 3, 2, 1 -- ordre INVERSE, garanti par la norme.
    for (usize index = 0U; index < 3U; ++index) {
        REQUIRE(mod03::LifetimeLog::entry(3U + index, event, tag));
        CHECK_EQ(event, Event::Destroy);
        CHECK_EQ(tag, 3 - static_cast<i32>(index));
    }
    CHECK(mod03::LifetimeLog::is_balanced());
}

TEST_REQ(CycleDeVie, equilibre_constructions_destructions, "LLR-M03-002") {
    reset();
    {
        const mod03::Traced a(10);
        {
            const mod03::Traced b(20);
            avio::unused(b);
        }
        const mod03::Traced c(30);
        avio::unused(a, c);
    }
    CHECK_EQ(mod03::LifetimeLog::count_of(Event::Construct), u32{3});
    CHECK_EQ(mod03::LifetimeLog::count_of(Event::Destroy), u32{3});
    CHECK(mod03::LifetimeLog::is_balanced());
}

TEST_REQ(CycleDeVie, copie_puis_deplacement, "LLR-M03-003") {
    reset();
    {
        mod03::Traced original(7);
        const mod03::Traced copy(original);              // constructeur de copie
        const mod03::Traced moved(std::move(original));  // constructeur de deplacement

        CHECK_EQ(copy.tag(), 7);
        CHECK_EQ(moved.tag(), 7);
        // Apres deplacement, la source est valide mais neutralisee.
        // DEVIATION JUSTIFIEE : lire un objet deplace est l'objet meme de ce
        // test (LLR-M03-003). Sans cette verification, rien ne prouverait que
        // la source a bien abandonne sa valeur.
        // NOLINTNEXTLINE(bugprone-use-after-move)
        CHECK_EQ(original.tag(), 0);
    }
    CHECK_EQ(mod03::LifetimeLog::count_of(Event::Construct), u32{1});
    CHECK_EQ(mod03::LifetimeLog::count_of(Event::Copy), u32{1});
    CHECK_EQ(mod03::LifetimeLog::count_of(Event::Move), u32{1});
    CHECK_EQ(mod03::LifetimeLog::count_of(Event::Destroy), u32{3});
    CHECK(mod03::LifetimeLog::is_balanced());
}

TEST_REQ(CycleDeVie, auto_affectation_sans_degat, "LLR-M03-004") {
    reset();
    mod03::Traced object(5);
    // On passe par un alias : l'auto-affectation directe declenche un
    // avertissement du compilateur, alors que le SCENARIO a tester (deux
    // references vers le meme objet) est bien reel en production.
    mod03::Traced& alias = object;
    object = alias;
    CHECK_EQ(object.tag(), 5);

    mod03::Traced other(6);
    other = std::move(alias);
    CHECK_EQ(other.tag(), 5);
}

// -----------------------------------------------------------------------------
//  RAII sur une ressource
// -----------------------------------------------------------------------------
TEST_REQ(RAII, acquisition_et_liberation_automatiques, "LLR-M03-010") {
    reset();
    CHECK_EQ(mod03::DeviceBank::acquired_count(), u8{0});
    {
        const mod03::ChannelHandle handle;
        REQUIRE(handle.is_valid());
        CHECK_EQ(mod03::DeviceBank::acquired_count(), u8{1});
        CHECK(mod03::DeviceBank::is_acquired(handle.channel()));
    }
    // Sortie de portee : le canal est libere sans aucune ligne de code dediee.
    CHECK_EQ(mod03::DeviceBank::acquired_count(), u8{0});
    CHECK_EQ(mod03::DeviceBank::total_acquisitions(), u32{1});
    CHECK_EQ(mod03::DeviceBank::total_releases(), u32{1});
}

TEST_REQ(RAII, epuisement_des_canaux, "LLR-M03-011") {
    reset();
    mod03::ChannelHandle p1;
    mod03::ChannelHandle p2;
    mod03::ChannelHandle p3;
    mod03::ChannelHandle p4;
    const mod03::ChannelHandle p5;  // il n'en reste plus

    CHECK(p1.is_valid());
    CHECK(p2.is_valid());
    CHECK(p3.is_valid());
    CHECK(p4.is_valid());
    CHECK_FALSE(p5.is_valid());
    CHECK_EQ(p5.channel(), u8{0});
    CHECK_EQ(mod03::DeviceBank::acquired_count(), mod03::DeviceBank::kChannelCount);
}

TEST_REQ(RAII, liberation_explicite_idempotente, "LLR-M03-012") {
    reset();
    mod03::ChannelHandle handle;
    REQUIRE(handle.is_valid());

    handle.release();
    CHECK_FALSE(handle.is_valid());
    CHECK_EQ(mod03::DeviceBank::acquired_count(), u8{0});

    handle.release();  // deuxieme appel : sans effet, pas de double liberation
    CHECK_EQ(mod03::DeviceBank::total_releases(), u32{1});
}

TEST_REQ(RAII, deplacement_transfere_la_propriete, "LLR-M03-013") {
    reset();
    {
        mod03::ChannelHandle source;
        REQUIRE(source.is_valid());
        const u8 channel = source.channel();

        const mod03::ChannelHandle destination(std::move(source));

        // NOLINTNEXTLINE(bugprone-use-after-move) -- exige par LLR-M03-013
        CHECK_FALSE(source.is_valid());  // la source a abandonne
        CHECK(destination.is_valid());
        CHECK_EQ(destination.channel(), channel);
        CHECK_EQ(mod03::DeviceBank::acquired_count(), u8{1});
    }
    // UNE seule liberation, malgre DEUX destructeurs appeles.
    CHECK_EQ(mod03::DeviceBank::total_releases(), u32{1});
    CHECK_EQ(mod03::DeviceBank::acquired_count(), u8{0});
}

TEST_REQ(RAII, affectation_par_deplacement_libere_l_ancienne, "LLR-M03-014") {
    reset();
    {
        mod03::ChannelHandle a;
        mod03::ChannelHandle b;
        REQUIRE(a.is_valid());
        REQUIRE(b.is_valid());
        CHECK_EQ(mod03::DeviceBank::acquired_count(), u8{2});

        a = std::move(b);  // l'ancien canal de `a` doit etre libere ici

        CHECK(a.is_valid());
        // NOLINTNEXTLINE(bugprone-use-after-move) -- exige par LLR-M03-014
        CHECK_FALSE(b.is_valid());
        CHECK_EQ(mod03::DeviceBank::acquired_count(), u8{1});
        CHECK_EQ(mod03::DeviceBank::total_releases(), u32{1});
    }
    CHECK_EQ(mod03::DeviceBank::acquired_count(), u8{0});
    CHECK_EQ(mod03::DeviceBank::total_acquisitions(), mod03::DeviceBank::total_releases());
}

TEST_REQ(RAII, robustesse_liberation_identifiant_invalide, "LLR-M03-015") {
    reset();
    mod03::DeviceBank::release(0U);
    mod03::DeviceBank::release(99U);
    CHECK_EQ(mod03::DeviceBank::total_releases(), u32{0});
    CHECK_FALSE(mod03::DeviceBank::is_acquired(0U));
    CHECK_FALSE(mod03::DeviceBank::is_acquired(99U));
}

// -----------------------------------------------------------------------------
//  Section critique
// -----------------------------------------------------------------------------
TEST_REQ(SectionCritique, interruptions_restaurees, "LLR-M03-020") {
    reset();
    CHECK(mod03::InterruptState::enabled());
    {
        const mod03::CriticalSection section;
        CHECK_FALSE(mod03::InterruptState::enabled());
    }
    CHECK(mod03::InterruptState::enabled());
}

TEST_REQ(SectionCritique, imbrication, "LLR-M03-021") {
    reset();
    {
        const mod03::CriticalSection external;
        {
            const mod03::CriticalSection internal;
            CHECK_FALSE(mod03::InterruptState::enabled());
        }
        // Sortie de la section INTERNE : les interruptions restent desactivees.
        CHECK_FALSE(mod03::InterruptState::enabled());
    }
    CHECK(mod03::InterruptState::enabled());
    CHECK_EQ(mod03::InterruptState::max_nesting(), u32{2});
}

TEST_REQ(SectionCritique, liberation_sur_tous_les_chemins, "LLR-M03-022") {
    // Le coeur du sujet : trois chemins de sortie differents, aucune ligne de
    // liberation ecrite a la main, et les interruptions sont restaurees dans
    // les trois cas. Sans RAII, il faudrait trois appels a enable(), et un
    // oubli sur une seule branche ne se verrait qu'a l'integration.
    reset();

    CHECK_EQ(mod03::multi_exit_processing(-5), 0);
    CHECK(mod03::InterruptState::enabled());

    CHECK_EQ(mod03::multi_exit_processing(0), 1);
    CHECK(mod03::InterruptState::enabled());

    CHECK_EQ(mod03::multi_exit_processing(7), 2);
    CHECK(mod03::InterruptState::enabled());

    CHECK_EQ(mod03::InterruptState::max_nesting(), u32{1});
}
