// =============================================================================
//  Module 08 -- demonstration : memoire statique, reserve de blocs, pile.
// =============================================================================
#include <avio/types.hpp>
#include <cstdio>

#include "mod08/memory_pool.hpp"
#include "mod08/stack_analysis.hpp"
#include "mod08/static_vector.hpp"

using avio::i32;
using avio::u32;
using avio::u64;
using avio::u8;
using avio::usize;

namespace {

void title(const char* text) {
    std::printf("\n=== %s ===\n", text);
}

// -----------------------------------------------------------------------------
void why_no_new() {
    title("Pourquoi `new` est interdit apres l'initialisation");
    std::printf("  1. NON-DETERMINISME TEMPOREL\n");
    std::printf("     La duree de `new` depend de l'historique complet des\n");
    std::printf("     allocations. Elle n'est pas bornable utilement.\n\n");
    std::printf("  2. FRAGMENTATION\n");
    std::printf("     Apres des heures de vol, la memoire libre peut etre\n");
    std::printf("     suffisante EN TOTAL mais morcelee. L'allocation echoue\n");
    std::printf("     alors qu'il \"reste de la place\". Ce defaut apparait apres\n");
    std::printf("     des dizaines d'heures : jamais pendant les tests.\n\n");
    std::printf("  3. ECHEC INGERABLE\n");
    std::printf("     Que faire si `new` echoue a 10 000 m ? Il n'y a pas de bonne\n");
    std::printf("     reponse. On rend donc l'echec IMPOSSIBLE.\n\n");
    std::printf("  4. DO-332 OO.6.8.2 : il faut DEMONTRER l'absence de fuite, de\n");
    std::printf("     fragmentation, d'epuisement et de reference pendante.\n");
    std::printf("     En allocation statique, c'est acquis par construction.\n");
}

// -----------------------------------------------------------------------------
void static_vector() {
    title("StaticVector : la capacite est dans le TYPE");

    mod08::StaticVector<i32, 4U> measurements;
    const i32 inputs[7] = {10, 20, 30, 40, 50, 60, 70};

    for (usize index = 0U; index < 7U; ++index) {
        const bool accepted = measurements.push_back(inputs[index]);
        std::printf("  push_back(%2d) -> %-8s taille=%zu\n", inputs[index],
                    accepted ? "accepte" : "REFUSE", measurements.size());
    }

    std::printf("\n  ajouts refuses : %u\n", measurements.rejected_count());
    std::printf("  contenu        :");
    const avio::Span<const i32> view = measurements.view();
    for (usize index = 0U; index < view.size(); ++index) {
        std::printf(" %d", view[index]);
    }
    std::printf("\n");

    std::printf("\n  std::vector aurait REALLOUE silencieusement. Ici le\n");
    std::printf("  depassement est un evenement OBSERVABLE et COMPTE : c'est le\n");
    std::printf("  signe que le dimensionnement merite d'etre reexamine.\n");
    std::printf("\n  Occupation RAM, connue a la compilation :\n");
    std::printf("    StaticVector<i32,   4>  -> %4zu octets\n",
                sizeof(mod08::StaticVector<i32, 4U>));
    std::printf("    StaticVector<i32, 100>  -> %4zu octets\n",
                sizeof(mod08::StaticVector<i32, 100U>));
}

// -----------------------------------------------------------------------------
void block_reserve() {
    title("MemoryPool : allouer sans `new`, en temps constant");

    mod08::MemoryPool<32U, 8U> pool;
    void* blocks[8] = {};

    std::printf("  taille d'un bloc : %zu octets, %zu blocs\n",
                mod08::MemoryPool<32U, 8U>::kBlockSize, mod08::MemoryPool<32U, 8U>::kBlockCount);
    std::printf("  occupation RAM totale : %zu octets\n", sizeof(pool));

    for (usize index = 0U; index < 8U; ++index) {
        blocks[index] = pool.allocate();
    }
    std::printf("\n  apres 8 allocations : libres=%zu, pic=%zu\n", pool.available(),
                pool.high_water_mark());
    std::printf("  9e allocation        : %s\n",
                (pool.allocate() == nullptr) ? "nullptr (epuisement detecte)" : "?!");

    std::printf("\n  Verifications de robustesse :\n");
    // ATTENTION : l ordre d evaluation des ARGUMENTS d un appel n est PAS
    // specifie en C++17. Ecrire les deux appels a deallocate() dans le meme
    // printf donnerait un resultat dependant du compilateur. On sequence donc
    // explicitement. Piege classique, et regle MISRA (au plus un effet de
    // bord par expression).
    const bool first = pool.deallocate(blocks[0]);
    const bool second = pool.deallocate(blocks[0]);
    std::printf("    liberer deux fois le meme bloc : %s puis %s\n", first ? "accepte" : "refuse",
                second ? "accepte" : "REFUSE (double liberation)");

    i32 local_variable = 0;
    std::printf("    liberer un pointeur etranger   : %s\n",
                pool.deallocate(&local_variable) ? "accepte" : "REFUSE");

    u8* middle = static_cast<u8*>(blocks[1]);
    std::printf("    liberer le milieu d'un bloc    : %s\n",
                pool.deallocate(middle + 8) ? "accepte" : "REFUSE");

    std::printf("\n  Aucune fragmentation possible : tous les blocs font la meme\n");
    std::printf("  taille. Allocation et liberation en O(1) : le WCET est borne.\n");
    std::printf("  C'est le principe des \"buffer pools\" d'un noyau ARINC 653.\n");
}

// -----------------------------------------------------------------------------
void stack_analysis() {
    title("Analyse de pile : pourquoi la recursion est bannie");

    mod08::CallDepthMonitor::reset();
    u64 result = 0U;
    (void)mod08::factorial(20U, result);
    const u32 iterative_depth = mod08::CallDepthMonitor::maximum();

    mod08::CallDepthMonitor::reset();
    (void)mod08::factorial_recursive(20U, result);
    const u32 recursive_depth = mod08::CallDepthMonitor::maximum();

    std::printf("  20! = %llu\n\n", static_cast<unsigned long long>(result));
    std::printf("  version iterative : profondeur de pile maximale = %u\n", iterative_depth);
    std::printf("  version recursive : profondeur de pile maximale = %u\n", recursive_depth);
    std::printf("\n  Meme resultat, %ux plus de pile. Sur une cible ou chaque trame\n",
                recursive_depth / ((iterative_depth == 0U) ? 1U : iterative_depth));
    std::printf("  fait 48 octets : 960 octets contre 48.\n");

    std::printf("\n  Ce qui rend une ANALYSE DE PILE possible :\n");
    std::printf("    * profondeur d'appel bornee et connue -> pas de recursion\n");
    std::printf("    * taille de trame connue              -> pas de VLA, pas d'alloca\n");
    std::printf("    * graphe d'appel statique             -> pas de pointeur de\n");
    std::printf("                                             fonction non resolu\n");
    std::printf("    * pas d'exceptions                    -> pas de trames de\n");
    std::printf("                                             deroulement imprevues\n");
    std::printf("\n  La plupart des interdits de ce cours convergent vers UN objectif :\n");
    std::printf("  rendre le graphe d'appel STATIQUEMENT ANALYSABLE.\n");
}

// -----------------------------------------------------------------------------
void memory_budget() {
    title("Le budget memoire : un livrable, pas une estimation");

    const usize vector_size = sizeof(mod08::StaticVector<i32, 100U>);
    const usize pool_size = sizeof(mod08::MemoryPool<32U, 8U>);
    const usize total = vector_size + pool_size;

    std::printf("  %-40s %8zu octets\n", "StaticVector<i32,100> (journal mesures)", vector_size);
    std::printf("  %-40s %8zu octets\n", "MemoryPool<32,8> (tampons messages)", pool_size);
    std::printf("  %-40s %8zu octets\n", "TOTAL alloue statiquement", total);
    std::printf("\n  Chaque octet de RAM de ce composant est connu AVANT l'execution.\n");
    std::printf("  On peut donc ecrire, dans le dossier de certification :\n");
    std::printf("    \"occupation RAM du composant : %zu octets, budget alloue : X ko,\n", total);
    std::printf("     marge : Y %%\" -- et le DEMONTRER par un simple sizeof.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 08 : memoire statique, reserve de blocs, pile      #\n");
    std::printf("#############################################################\n");

    why_no_new();
    static_vector();
    block_reserve();
    stack_analysis();
    memory_budget();

    std::printf("\nModule 08 termine.\n");
    return 0;
}
