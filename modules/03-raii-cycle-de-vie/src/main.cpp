// =============================================================================
//  Module 03 -- demonstration : RAII et cycle de vie.
// =============================================================================
#include <avio/types.hpp>
#include <cstdio>
#include <utility>

#include "mod03/lifetime.hpp"

using avio::i32;
using avio::usize;
using Event = mod03::LifetimeLog::Event;

namespace {

void title(const char* text) {
    std::printf("\n=== %s ===\n", text);
}

void print_log() {
    for (usize index = 0U; index < mod03::LifetimeLog::count(); ++index) {
        Event event = Event::Construct;
        i32 tag = 0;
        if (mod03::LifetimeLog::entry(index, event, tag)) {
            std::printf("    %2zu. %-28s objet #%d\n", index + 1U,
                        mod03::LifetimeLog::event_name(event), tag);
        }
    }
    std::printf("    equilibre construction/destruction : %s\n",
                mod03::LifetimeLog::is_balanced() ? "OUI" : "NON");
}

// -----------------------------------------------------------------------------
void destruction_order() {
    title("Ordre de destruction : inverse de la construction");
    mod03::LifetimeLog::reset();
    mod03::demonstrate_destruction_order();
    print_log();
    std::printf("\n  Garanti par la norme, donc utilisable comme propriete de conception :\n");
    std::printf("  un membre declare APRES un autre est detruit AVANT lui.\n");
}

// -----------------------------------------------------------------------------
void copy_and_move() {
    title("Copie contre deplacement");
    mod03::LifetimeLog::reset();
    {
        mod03::Traced original(7);
        const mod03::Traced copy(original);
        const mod03::Traced moved(std::move(original));
        std::printf("    tag de la copie          : %d\n", copy.tag());
        std::printf("    tag du deplace           : %d\n", moved.tag());
        // DEVIATION JUSTIFIEE (bugprone-use-after-move) : lire un objet deplace
        // est normalement suspect. Ici, c'est precisement le COMPORTEMENT
        // SPECIFIE que l'on veut montrer (LLR-M03-003 : la source est
        // neutralisee). Derogation locale, tracee, limitee a une ligne.
        // NOLINTNEXTLINE(bugprone-use-after-move)
        std::printf("    tag de la source apres move : %d  (neutralisee)\n", original.tag());
    }
    print_log();
    std::printf("\n  `std::move` ne DEPLACE rien : c'est un simple cast qui autorise\n");
    std::printf("  le constructeur de deplacement a piller la source. C'est votre\n");
    std::printf("  code qui doit laisser la source dans un etat valide.\n");
}

// -----------------------------------------------------------------------------
void raii_resource() {
    title("RAII : la ressource se libere toute seule");
    mod03::DeviceBank::reset();

    std::printf("  canaux occupes au depart : %u\n", mod03::DeviceBank::acquired_count());
    {
        const mod03::ChannelHandle handle;
        std::printf("  dans la portee : canal #%u, occupes = %u\n", handle.channel(),
                    mod03::DeviceBank::acquired_count());
    }
    std::printf("  apres la portee : occupes = %u (aucune ligne de liberation ecrite)\n",
                mod03::DeviceBank::acquired_count());

    std::printf("\n  Transfert de propriete :\n");
    mod03::DeviceBank::reset();
    {
        mod03::ChannelHandle source;
        std::printf("    source detient le canal #%u\n", source.channel());
        const mod03::ChannelHandle destination(std::move(source));
        // NOLINTNEXTLINE(bugprone-use-after-move) -- etat post-deplacement specifie
        const bool valid_source = source.is_valid();
        std::printf("    apres move : source valide = %s, destination = canal #%u\n",
                    valid_source ? "oui" : "non", destination.channel());
    }
    std::printf("    deux destructeurs appeles, UNE seule liberation : %u\n",
                mod03::DeviceBank::total_releases());
}

// -----------------------------------------------------------------------------
void critical_section() {
    title("Section critique : liberation sur TOUS les chemins de sortie");
    mod03::InterruptState::reset();

    const i32 inputs[3] = {-5, 0, 7};
    for (usize index = 0U; index < 3U; ++index) {
        const i32 result = mod03::multi_exit_processing(inputs[index]);
        std::printf("  traitement(%2d) -> %d ; interruptions actives apres : %s\n", inputs[index],
                    result, mod03::InterruptState::enabled() ? "OUI" : "NON");
    }

    std::printf("\n  Sans RAII, il faudrait un enable() sur chacune des 3 sorties.\n");
    std::printf("  Un oubli sur une seule branche : les interruptions restent coupees,\n");
    std::printf("  le chien de garde se declenche, le calculateur redemarre en vol.\n");
    std::printf("  Ce defaut ne se voit ni a la relecture rapide, ni en test nominal.\n");
}

// -----------------------------------------------------------------------------
void csharp_comparison() {
    title("Comparaison avec C#");
    std::printf("  C#                                   C++\n");
    std::printf("  ----------------------------------   --------------------------------\n");
    std::printf("  using (var l = new Lock()) { }       { ScopedLock l; }\n");
    std::printf("  -> il faut PENSER a ecrire `using`   -> impossible a oublier\n\n");
    std::printf("  ~Finalizer()                         ~Destructeur()\n");
    std::printf("  -> appele un jour, par le GC         -> appele MAINTENANT, ici\n");
    std::printf("  -> sur un thread quelconque          -> sur le thread courant\n");
    std::printf("  -> peut ne jamais etre appele        -> garanti par la norme\n\n");
    std::printf("  C'est ce DETERMINISME que recherche l'avionique : on doit pouvoir\n");
    std::printf("  affirmer, preuve a l'appui, quand une ressource est liberee.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 03 : RAII et cycle de vie                          #\n");
    std::printf("#############################################################\n");

    destruction_order();
    copy_and_move();
    raii_resource();
    critical_section();
    csharp_comparison();

    std::printf("\nModule 03 termine.\n");
    return 0;
}
