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

void handler_silencieux(const char* condition, const char* file, avio::i32 line) noexcept {
    avio::unused(condition, file, line);
}

}  // namespace

// =============================================================================
//  StaticVector
// =============================================================================
TEST_REQ(StaticVector, etat_initial, "LLR-M08-001") {
    const mod08::StaticVector<i32, 4U> vecteur;
    CHECK(vecteur.empty());
    CHECK_FALSE(vecteur.full());
    CHECK_EQ(vecteur.size(), usize{0});
    CHECK_EQ(vecteur.capacity(), usize{4});
    CHECK_EQ(vecteur.rejected_count(), u32{0});
}

TEST_REQ(StaticVector, ajout_et_retrait, "LLR-M08-002") {
    mod08::StaticVector<i32, 4U> vecteur;
    CHECK(vecteur.push_back(10));
    CHECK(vecteur.push_back(20));
    CHECK_EQ(vecteur.size(), usize{2});
    CHECK_EQ(vecteur[0], 10);
    CHECK_EQ(vecteur[1], 20);

    i32 valeur = 0;
    REQUIRE(vecteur.pop_back(valeur));
    CHECK_EQ(valeur, 20);
    CHECK_EQ(vecteur.size(), usize{1});
}

TEST_REQ(StaticVector, capacite_saturee_sans_realloc, "LLR-M08-003") {
    // Point cle : contrairement a std::vector, il n'y a AUCUNE reallocation.
    // Le depassement est un evenement observable, pas un appel silencieux
    // a l'allocateur.
    mod08::StaticVector<i32, 3U> vecteur;
    CHECK(vecteur.push_back(1));
    CHECK(vecteur.push_back(2));
    CHECK(vecteur.push_back(3));
    CHECK(vecteur.full());

    CHECK_FALSE(vecteur.push_back(4));
    CHECK_FALSE(vecteur.push_back(5));
    CHECK_EQ(vecteur.size(), usize{3});
    CHECK_EQ(vecteur.rejected_count(), u32{2});
    CHECK_EQ(vecteur[2], 3);  // le contenu n'a pas ete altere
}

TEST_REQ(StaticVector, robustesse_retrait_sur_vide, "LLR-M08-004") {
    mod08::StaticVector<i32, 4U> vecteur;
    i32 valeur = 777;
    CHECK_FALSE(vecteur.pop_back(valeur));
    CHECK_EQ(valeur, 777);
}

TEST_REQ(StaticVector, suppression_par_indice, "LLR-M08-005") {
    mod08::StaticVector<i32, 5U> vecteur;
    for (i32 index = 0; index < 5; ++index) {
        (void)vecteur.push_back(index * 10);
    }
    CHECK(vecteur.erase(1U));  // retire 10
    CHECK_EQ(vecteur.size(), usize{4});
    CHECK_EQ(vecteur[0], 0);
    CHECK_EQ(vecteur[1], 20);
    CHECK_EQ(vecteur[3], 40);

    CHECK_FALSE(vecteur.erase(4U));  // hors domaine
    CHECK_FALSE(vecteur.erase(100U));
    CHECK_EQ(vecteur.size(), usize{4});
}

TEST_REQ(StaticVector, acces_verifie_et_non_verifie, "LLR-M08-006") {
    mod08::StaticVector<i32, 4U> vecteur;
    (void)vecteur.push_back(42);

    i32 valeur = 0;
    CHECK(vecteur.at(0U, valeur));
    CHECK_EQ(valeur, 42);

    valeur = 999;
    CHECK_FALSE(vecteur.at(1U, valeur));
    CHECK_EQ(valeur, 999);

    // operator[] hors domaine : le gestionnaire d'anomalie est notifie et la
    // valeur renvoyee reste deterministe.
    avio::reset_fault_count();
    const avio::FaultHandler precedent = avio::set_fault_handler(&handler_silencieux);
    const i32 lu = vecteur[5U];
    avio::set_fault_handler(precedent);
    CHECK_EQ(avio::fault_count(), u32{1});
    CHECK_EQ(lu, 42);  // repli sur l'element 0, jamais de lecture sauvage
}

TEST_REQ(StaticVector, vue_limitee_a_la_taille, "LLR-M08-007") {
    mod08::StaticVector<i32, 8U> vecteur;
    (void)vecteur.push_back(1);
    (void)vecteur.push_back(2);

    const avio::Span<const i32> vue = vecteur.view();
    CHECK_EQ(vue.size(), usize{2});  // pas 8 : la vue expose la TAILLE
    CHECK_EQ(vue[0], 1);
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
    void* bloc = pool.allocate();
    REQUIRE(bloc != nullptr);
    CHECK_EQ(pool.in_use(), usize{1});
    CHECK(pool.owns(bloc));

    CHECK(pool.deallocate(bloc));
    CHECK_EQ(pool.in_use(), usize{0});
    CHECK_EQ(pool.available(), usize{8});
}

TEST_REQ(MemoryPool, epuisement_borne_et_detectable, "LLR-M08-012") {
    Pool pool;
    void* blocs[8] = {};
    for (usize index = 0U; index < 8U; ++index) {
        blocs[index] = pool.allocate();
        CHECK(blocs[index] != nullptr);
    }
    CHECK_EQ(pool.available(), usize{0});

    // L'epuisement renvoie nullptr : jamais d'exception, jamais d'attente.
    CHECK(pool.allocate() == nullptr);
    CHECK_EQ(pool.high_water_mark(), usize{8});

    for (usize index = 0U; index < 8U; ++index) {
        CHECK(pool.deallocate(blocs[index]));
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
    const usize ecart = static_cast<usize>((pb > pa) ? (pb - pa) : (pa - pb));
    CHECK_EQ(ecart, Pool::kBlockSize);
}

TEST_REQ(MemoryPool, double_liberation_detectee, "LLR-M08-014") {
    // Sans cette detection, la seconde liberation reinsererait le meme bloc
    // dans la liste des libres : deux allocations futures renverraient LE MEME
    // bloc. C'est l'un des defauts memoire les plus difficiles a diagnostiquer.
    Pool pool;
    void* bloc = pool.allocate();
    REQUIRE(bloc != nullptr);

    CHECK(pool.deallocate(bloc));
    CHECK_FALSE(pool.deallocate(bloc));
    CHECK_EQ(pool.available(), usize{8});
}

TEST_REQ(MemoryPool, pointeur_etranger_refuse, "LLR-M08-015") {
    Pool pool;
    i32 variable_locale = 0;
    CHECK_FALSE(pool.deallocate(&variable_locale));
    CHECK_FALSE(pool.deallocate(nullptr));
    CHECK_FALSE(pool.owns(&variable_locale));
}

TEST_REQ(MemoryPool, pointeur_mal_aligne_refuse, "LLR-M08-016") {
    Pool pool;
    u8* bloc = static_cast<u8*>(pool.allocate());
    REQUIRE(bloc != nullptr);

    // Pointeur au MILIEU d'un bloc : appartient a la reserve, mais n'est pas
    // un debut de bloc valide.
    CHECK_FALSE(pool.deallocate(bloc + 4));
    CHECK_EQ(pool.in_use(), usize{1});
    CHECK(pool.deallocate(bloc));
}

TEST_REQ(MemoryPool, reutilisation_sans_fragmentation, "LLR-M08-017") {
    // Sequence qui fragmenterait un tas classique : allocations et liberations
    // entrelacees. Ici, tous les blocs ont la meme taille : la fragmentation
    // est IMPOSSIBLE par construction.
    Pool pool;
    void* blocs[8] = {};

    for (usize cycle = 0U; cycle < 50U; ++cycle) {
        for (usize index = 0U; index < 8U; ++index) {
            blocs[index] = pool.allocate();
            REQUIRE(blocs[index] != nullptr);
        }
        // Liberation dans un ordre volontairement irregulier.
        CHECK(pool.deallocate(blocs[3]));
        CHECK(pool.deallocate(blocs[0]));
        CHECK(pool.deallocate(blocs[6]));
        CHECK(pool.deallocate(blocs[1]));
        CHECK(pool.deallocate(blocs[7]));
        CHECK(pool.deallocate(blocs[4]));
        CHECK(pool.deallocate(blocs[2]));
        CHECK(pool.deallocate(blocs[5]));
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
    u64 resultat = 0U;

    REQUIRE(mod08::factorial(5U, resultat));
    CHECK_EQ(resultat, u64{120});
    CHECK_EQ(mod08::CallDepthMonitor::maximum(), u32{1});

    REQUIRE(mod08::factorial(20U, resultat));
    CHECK_EQ(resultat, u64{2432902008176640000});
    // Profondeur toujours 1, quelle que soit l'entree.
    CHECK_EQ(mod08::CallDepthMonitor::maximum(), u32{1});
}

TEST_REQ(Pile, factorielle_robustesse_hors_domaine, "LLR-M08-021") {
    u64 resultat = 42U;
    CHECK_FALSE(mod08::factorial(21U, resultat));
    CHECK_EQ(resultat, u64{0});
    CHECK_FALSE(mod08::factorial(1000U, resultat));
}

TEST_REQ(Pile, recursion_profondeur_proportionnelle, "LLR-M08-022") {
    // Le contre-exemple : meme resultat, mais la profondeur de pile suit n.
    mod08::CallDepthMonitor::reset();
    u64 resultat = 0U;

    REQUIRE(mod08::factorial_recursive(5U, resultat));
    CHECK_EQ(resultat, u64{120});
    CHECK_EQ(mod08::CallDepthMonitor::maximum(), u32{5});

    mod08::CallDepthMonitor::reset();
    REQUIRE(mod08::factorial_recursive(20U, resultat));
    CHECK_EQ(resultat, u64{2432902008176640000});
    CHECK_EQ(mod08::CallDepthMonitor::maximum(), u32{20});

    // Vingt fois plus de pile pour le meme resultat. Sur une cible ou chaque
    // trame fait 48 octets, cela fait 960 octets contre 48.
}

TEST_REQ(Pile, les_deux_versions_concordent, "LLR-M08-023") {
    for (u32 n = 0U; n <= 20U; ++n) {
        u64 iteratif = 0U;
        u64 recursif = 0U;
        REQUIRE(mod08::factorial(n, iteratif));
        REQUIRE(mod08::factorial_recursive(n, recursif));
        CHECK_EQ(iteratif, recursif);
    }
}

TEST_REQ(Pile, profondeur_revient_a_zero, "LLR-M08-024") {
    // La garde RAII decremente sur TOUS les chemins, y compris les sorties
    // anticipees pour entree invalide.
    mod08::CallDepthMonitor::reset();
    u64 resultat = 0U;
    (void)mod08::factorial_recursive(10U, resultat);
    (void)mod08::factorial_recursive(99U, resultat);  // sortie anticipee
    CHECK_EQ(mod08::CallDepthMonitor::current(), u32{0});
}

TEST_REQ(Pile, somme_iterative, "LLR-M08-025") {
    const i32 valeurs[5] = {1, 2, 3, 4, 5};
    CHECK_EQ(mod08::sum_iterative(valeurs, 5U), i64{15});
    CHECK_EQ(mod08::sum_iterative(nullptr, 5U), i64{0});
    CHECK_EQ(mod08::sum_iterative(valeurs, 0U), i64{0});
}

TEST_REQ(Pile, recherche_dichotomique, "LLR-M08-026") {
    const i32 trie[7] = {-10, -3, 0, 4, 9, 21, 100};
    usize index = 0U;

    REQUIRE(mod08::binary_search(trie, 7U, 0, index));
    CHECK_EQ(index, usize{2});
    REQUIRE(mod08::binary_search(trie, 7U, -10, index));
    CHECK_EQ(index, usize{0});
    REQUIRE(mod08::binary_search(trie, 7U, 100, index));
    CHECK_EQ(index, usize{6});

    CHECK_FALSE(mod08::binary_search(trie, 7U, 5, index));
    CHECK_FALSE(mod08::binary_search(trie, 0U, 0, index));
    CHECK_FALSE(mod08::binary_search(nullptr, 7U, 0, index));
}
