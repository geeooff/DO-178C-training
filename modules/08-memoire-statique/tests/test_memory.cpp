#include <avio/assert.hpp>
#include <avio/types.hpp>
#include <microtest/microtest.hpp>

#include "mod08/memory_pool.hpp"
#include "mod08/stack_analysis.hpp"
#include "mod08/static_vector.hpp"

using avio::i32;
using avio::i64;
using avio::u32;
using avio::u64;
using avio::u8;
using avio::usize;

namespace {

void silent_handler(const char* condition, const char* file, avio::i32 line) noexcept {
    avio::unused(condition, file, line);
}

}  // namespace

// =============================================================================
//  StaticVector
// =============================================================================
TEST_REQ(StaticVector, etat_initial, "LLR-M08-001") {
    const mod08::StaticVector<i32, 4U> vector;
    CHECK(vector.empty());
    CHECK_FALSE(vector.full());
    CHECK_EQ(vector.size(), usize{0});
    CHECK_EQ(vector.capacity(), usize{4});
    CHECK_EQ(vector.rejected_count(), u32{0});
}

TEST_REQ(StaticVector, ajout_et_retrait, "LLR-M08-002") {
    mod08::StaticVector<i32, 4U> vector;
    CHECK(vector.push_back(10));
    CHECK(vector.push_back(20));
    CHECK_EQ(vector.size(), usize{2});
    CHECK_EQ(vector[0], 10);
    CHECK_EQ(vector[1], 20);

    i32 value = 0;
    REQUIRE(vector.pop_back(value));
    CHECK_EQ(value, 20);
    CHECK_EQ(vector.size(), usize{1});
}

TEST_REQ(StaticVector, capacite_saturee_sans_realloc, "LLR-M08-003") {
    // Point cle : contrairement a std::vector, il n'y a AUCUNE reallocation.
    // Le depassement est un evenement observable, pas un appel silencieux
    // a l'allocateur.
    mod08::StaticVector<i32, 3U> vector;
    CHECK(vector.push_back(1));
    CHECK(vector.push_back(2));
    CHECK(vector.push_back(3));
    CHECK(vector.full());

    CHECK_FALSE(vector.push_back(4));
    CHECK_FALSE(vector.push_back(5));
    CHECK_EQ(vector.size(), usize{3});
    CHECK_EQ(vector.rejected_count(), u32{2});
    CHECK_EQ(vector[2], 3);  // le contenu n'a pas ete altere
}

TEST_REQ(StaticVector, robustesse_retrait_sur_vide, "LLR-M08-004") {
    mod08::StaticVector<i32, 4U> vector;
    i32 value = 777;
    CHECK_FALSE(vector.pop_back(value));
    CHECK_EQ(value, 777);
}

TEST_REQ(StaticVector, suppression_par_indice, "LLR-M08-005") {
    mod08::StaticVector<i32, 5U> vector;
    for (i32 index = 0; index < 5; ++index) {
        (void)vector.push_back(index * 10);
    }
    CHECK(vector.erase(1U));  // retire 10
    CHECK_EQ(vector.size(), usize{4});
    CHECK_EQ(vector[0], 0);
    CHECK_EQ(vector[1], 20);
    CHECK_EQ(vector[3], 40);

    CHECK_FALSE(vector.erase(4U));  // hors domaine
    CHECK_FALSE(vector.erase(100U));
    CHECK_EQ(vector.size(), usize{4});
}

TEST_REQ(StaticVector, acces_verifie_et_non_verifie, "LLR-M08-006") {
    mod08::StaticVector<i32, 4U> vector;
    (void)vector.push_back(42);

    i32 value = 0;
    CHECK(vector.at(0U, value));
    CHECK_EQ(value, 42);

    value = 999;
    CHECK_FALSE(vector.at(1U, value));
    CHECK_EQ(value, 999);

    // operator[] hors domaine : le gestionnaire d'anomalie est notifie et la
    // valeur renvoyee reste deterministe.
    avio::reset_fault_count();
    const avio::FaultHandler previous = avio::set_fault_handler(&silent_handler);
    const i32 lu = vector[5U];
    avio::set_fault_handler(previous);
    CHECK_EQ(avio::fault_count(), u32{1});
    CHECK_EQ(lu, 42);  // repli sur l'element 0, jamais de lecture sauvage
}

TEST_REQ(StaticVector, vue_limitee_a_la_taille, "LLR-M08-007") {
    mod08::StaticVector<i32, 8U> vector;
    (void)vector.push_back(1);
    (void)vector.push_back(2);

    const avio::Span<const i32> view = vector.view();
    CHECK_EQ(view.size(), usize{2});  // pas 8 : la vue expose la TAILLE
    CHECK_EQ(view[0], 1);
}

TEST_REQ(StaticVector, occupation_memoire_connue, "LLR-M08-008") {
    // L'occupation RAM est une CONSTANTE, verifiable a la compilation et
    // inscriptible au budget memoire du calculateur.
    CHECK(sizeof(mod08::StaticVector<i32, 100U>) >= (100U * sizeof(i32)));
    CHECK(sizeof(mod08::StaticVector<i32, 100U>) < ((100U * sizeof(i32)) + 64U));
}

// =============================================================================
//  MemoryPool
// =============================================================================
using Pool = mod08::MemoryPool<32U, 8U>;

TEST_REQ(MemoryPool, etat_initial, "LLR-M08-010") {
    Pool pool;
    CHECK_EQ(pool.in_use(), usize{0});
    CHECK_EQ(pool.available(), usize{8});
    CHECK_EQ(pool.high_water_mark(), usize{0});
}

TEST_REQ(MemoryPool, allocation_et_liberation, "LLR-M08-011") {
    Pool pool;
    void* block = pool.allocate();
    REQUIRE(block != nullptr);
    CHECK_EQ(pool.in_use(), usize{1});
    CHECK(pool.owns(block));

    CHECK(pool.deallocate(block));
    CHECK_EQ(pool.in_use(), usize{0});
    CHECK_EQ(pool.available(), usize{8});
}

TEST_REQ(MemoryPool, epuisement_borne_et_detectable, "LLR-M08-012") {
    Pool pool;
    void* blocks[8] = {};
    for (usize index = 0U; index < 8U; ++index) {
        blocks[index] = pool.allocate();
        CHECK(blocks[index] != nullptr);
    }
    CHECK_EQ(pool.available(), usize{0});

    // L'epuisement renvoie nullptr : jamais d'exception, jamais d'attente.
    CHECK(pool.allocate() == nullptr);
    CHECK_EQ(pool.high_water_mark(), usize{8});

    for (usize index = 0U; index < 8U; ++index) {
        CHECK(pool.deallocate(blocks[index]));
    }
    CHECK_EQ(pool.available(), usize{8});
    // Le pic reste memorise : c'est LA metrique de dimensionnement.
    CHECK_EQ(pool.high_water_mark(), usize{8});
}

TEST_REQ(MemoryPool, blocs_distincts_et_alignes, "LLR-M08-013") {
    Pool pool;
    void* a = pool.allocate();
    void* b = pool.allocate();
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(a != b);

    const u8* pa = static_cast<const u8*>(a);
    const u8* pb = static_cast<const u8*>(b);
    const usize delta = static_cast<usize>((pb > pa) ? (pb - pa) : (pa - pb));
    CHECK_EQ(delta, Pool::kBlockSize);
}

TEST_REQ(MemoryPool, double_liberation_detectee, "LLR-M08-014") {
    // Sans cette detection, la seconde liberation reinsererait le meme bloc
    // dans la liste des libres : deux allocations futures renverraient LE MEME
    // bloc. C'est l'un des defauts memoire les plus difficiles a diagnostiquer.
    Pool pool;
    void* block = pool.allocate();
    REQUIRE(block != nullptr);

    CHECK(pool.deallocate(block));
    CHECK_FALSE(pool.deallocate(block));
    CHECK_EQ(pool.available(), usize{8});
}

TEST_REQ(MemoryPool, pointeur_etranger_refuse, "LLR-M08-015") {
    Pool pool;
    i32 local_variable = 0;
    CHECK_FALSE(pool.deallocate(&local_variable));
    CHECK_FALSE(pool.deallocate(nullptr));
    CHECK_FALSE(pool.owns(&local_variable));
}

TEST_REQ(MemoryPool, pointeur_mal_aligne_refuse, "LLR-M08-016") {
    Pool pool;
    u8* block = static_cast<u8*>(pool.allocate());
    REQUIRE(block != nullptr);

    // Pointeur au MILIEU d'un bloc : appartient a la reserve, mais n'est pas
    // un debut de bloc valide.
    CHECK_FALSE(pool.deallocate(block + 4));
    CHECK_EQ(pool.in_use(), usize{1});
    CHECK(pool.deallocate(block));
}

TEST_REQ(MemoryPool, reutilisation_sans_fragmentation, "LLR-M08-017") {
    // Sequence qui fragmenterait un tas classique : allocations et liberations
    // entrelacees. Ici, tous les blocs ont la meme taille : la fragmentation
    // est IMPOSSIBLE par construction.
    Pool pool;
    void* blocks[8] = {};

    for (usize cycle = 0U; cycle < 50U; ++cycle) {
        for (usize index = 0U; index < 8U; ++index) {
            blocks[index] = pool.allocate();
            REQUIRE(blocks[index] != nullptr);
        }
        // Liberation dans un ordre volontairement irregulier.
        CHECK(pool.deallocate(blocks[3]));
        CHECK(pool.deallocate(blocks[0]));
        CHECK(pool.deallocate(blocks[6]));
        CHECK(pool.deallocate(blocks[1]));
        CHECK(pool.deallocate(blocks[7]));
        CHECK(pool.deallocate(blocks[4]));
        CHECK(pool.deallocate(blocks[2]));
        CHECK(pool.deallocate(blocks[5]));
        REQUIRE_EQ(pool.available(), usize{8});
    }
    // Apres 400 allocations, la reserve est exactement dans son etat initial.
    CHECK_EQ(pool.in_use(), usize{0});
    CHECK_EQ(pool.high_water_mark(), usize{8});
}

// =============================================================================
//  Analyse de pile
// =============================================================================
TEST_REQ(Pile, factorielle_iterative_profondeur_constante, "LLR-M08-020") {
    mod08::CallDepthMonitor::reset();
    u64 result = 0U;

    REQUIRE(mod08::factorial(5U, result));
    CHECK_EQ(result, u64{120});
    CHECK_EQ(mod08::CallDepthMonitor::maximum(), u32{1});

    REQUIRE(mod08::factorial(20U, result));
    CHECK_EQ(result, u64{2432902008176640000});
    // Profondeur toujours 1, quelle que soit l'entree.
    CHECK_EQ(mod08::CallDepthMonitor::maximum(), u32{1});
}

TEST_REQ(Pile, factorielle_robustesse_hors_domaine, "LLR-M08-021") {
    u64 result = 42U;
    CHECK_FALSE(mod08::factorial(21U, result));
    CHECK_EQ(result, u64{0});
    CHECK_FALSE(mod08::factorial(1000U, result));
}

TEST_REQ(Pile, recursion_profondeur_proportionnelle, "LLR-M08-022") {
    // Le contre-exemple : meme resultat, mais la profondeur de pile suit n.
    mod08::CallDepthMonitor::reset();
    u64 result = 0U;

    REQUIRE(mod08::factorial_recursive(5U, result));
    CHECK_EQ(result, u64{120});
    CHECK_EQ(mod08::CallDepthMonitor::maximum(), u32{5});

    mod08::CallDepthMonitor::reset();
    REQUIRE(mod08::factorial_recursive(20U, result));
    CHECK_EQ(result, u64{2432902008176640000});
    CHECK_EQ(mod08::CallDepthMonitor::maximum(), u32{20});

    // Vingt fois plus de pile pour le meme resultat. Sur une cible ou chaque
    // trame fait 48 octets, cela fait 960 octets contre 48.
}

TEST_REQ(Pile, les_deux_versions_concordent, "LLR-M08-023") {
    for (u32 n = 0U; n <= 20U; ++n) {
        u64 iterative = 0U;
        u64 recursive = 0U;
        REQUIRE(mod08::factorial(n, iterative));
        REQUIRE(mod08::factorial_recursive(n, recursive));
        CHECK_EQ(iterative, recursive);
    }
}

TEST_REQ(Pile, profondeur_revient_a_zero, "LLR-M08-024") {
    // La garde RAII decremente sur TOUS les chemins, y compris les sorties
    // anticipees pour entree invalide.
    mod08::CallDepthMonitor::reset();
    u64 result = 0U;
    (void)mod08::factorial_recursive(10U, result);
    (void)mod08::factorial_recursive(99U, result);  // sortie anticipee
    CHECK_EQ(mod08::CallDepthMonitor::current(), u32{0});
}

TEST_REQ(Pile, somme_iterative, "LLR-M08-025") {
    const i32 values[5] = {1, 2, 3, 4, 5};
    CHECK_EQ(mod08::sum_iterative(values, 5U), i64{15});
    CHECK_EQ(mod08::sum_iterative(nullptr, 5U), i64{0});
    CHECK_EQ(mod08::sum_iterative(values, 0U), i64{0});
}

TEST_REQ(Pile, recherche_dichotomique, "LLR-M08-026") {
    const i32 sorted[7] = {-10, -3, 0, 4, 9, 21, 100};
    usize index = 0U;

    REQUIRE(mod08::binary_search(sorted, 7U, 0, index));
    CHECK_EQ(index, usize{2});
    REQUIRE(mod08::binary_search(sorted, 7U, -10, index));
    CHECK_EQ(index, usize{0});
    REQUIRE(mod08::binary_search(sorted, 7U, 100, index));
    CHECK_EQ(index, usize{6});

    CHECK_FALSE(mod08::binary_search(sorted, 7U, 5, index));
    CHECK_FALSE(mod08::binary_search(sorted, 0U, 0, index));
    CHECK_FALSE(mod08::binary_search(nullptr, 7U, 0, index));
}
