// =============================================================================
//  microtest -- moteur d'execution.
//
//  Ce fichier fournit `main`. Il est compile dans la bibliotheque statique
//  `microtest` : chaque executable de test se contente de declarer ses cas
//  avec TEST / TEST_REQ, l'editeur de liens tire ce `main` automatiquement.
//
//  Options de ligne de commande :
//     --list            liste les cas de test sans les executer
//     --filter=<texte>  n'execute que les cas dont "suite.nom" contient <texte>
//     --req             affiche la matrice exigence -> cas de test
//     --csv=<fichier>   exporte la tracabilite au format CSV (preuve archivable)
//     --verbose         affiche chaque cas meme en cas de succes
// =============================================================================
#include <microtest/microtest.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

bool starts_with(const char* text, const char* prefix) noexcept {
    return std::strncmp(text, prefix, std::strlen(prefix)) == 0;
}

bool contains(const char* haystack, const char* needle) noexcept {
    return std::strstr(haystack, needle) != nullptr;
}

/// Construit "suite.nom" dans un tampon fourni par l'appelant.
void full_name(char* buffer, std::size_t size, const microtest::TestCase& tc) noexcept {
    std::snprintf(buffer, size, "%s.%s", tc.suite, tc.name);
}

/// Applique `callback` a chaque identifiant d'exigence d'une liste "A,B,C".
template <typename Callback>
void for_each_requirement(const char* list, Callback callback) {
    if (list == nullptr) {
        return;
    }
    const char* cursor = list;
    while (*cursor != '\0') {
        while ((*cursor == ' ') || (*cursor == ',')) {
            ++cursor;
        }
        const char* start = cursor;
        while ((*cursor != '\0') && (*cursor != ',')) {
            ++cursor;
        }
        std::size_t length = static_cast<std::size_t>(cursor - start);
        while ((length > 0U) && (start[length - 1U] == ' ')) {
            --length;
        }
        if (length > 0U) {
            char identifier[96];
            const std::size_t copied = (length < sizeof(identifier) - 1U) ? length
                                                                          : sizeof(identifier) - 1U;
            std::memcpy(identifier, start, copied);
            identifier[copied] = '\0';
            callback(static_cast<const char*>(identifier));
        }
    }
}

void print_requirement_matrix(const microtest::Registry& registry) {
    std::printf("\n--- Matrice de tracabilite : exigence -> cas de test ---\n");

    // Table statique des exigences deja affichees (pas d'allocation dynamique).
    constexpr std::size_t kMaxRequirements = 256U;
    static char seen[kMaxRequirements][96];
    std::size_t seen_count = 0U;

    for (std::size_t i = 0U; i < registry.size(); ++i) {
        for_each_requirement(registry.at(i).requirements, [&](const char* req) {
            for (std::size_t k = 0U; k < seen_count; ++k) {
                if (std::strcmp(seen[k], req) == 0) {
                    return;
                }
            }
            if (seen_count < kMaxRequirements) {
                std::snprintf(seen[seen_count], sizeof(seen[0]), "%s", req);
                ++seen_count;
            }
        });
    }

    for (std::size_t k = 0U; k < seen_count; ++k) {
        std::printf("  %-24s :", seen[k]);
        for (std::size_t i = 0U; i < registry.size(); ++i) {
            for_each_requirement(registry.at(i).requirements, [&](const char* req) {
                if (std::strcmp(seen[k], req) == 0) {
                    std::printf(" %s.%s", registry.at(i).suite, registry.at(i).name);
                }
            });
        }
        std::printf("\n");
    }

    std::size_t untraced = 0U;
    for (std::size_t i = 0U; i < registry.size(); ++i) {
        if (registry.at(i).requirements == nullptr) {
            ++untraced;
        }
    }
    std::printf("  %zu exigence(s) couverte(s), %zu cas de test SANS tracabilite\n", seen_count,
                untraced);
}

void export_csv(const microtest::Registry& registry, const char* path) {
    std::FILE* file = std::fopen(path, "w");
    if (file == nullptr) {
        std::printf("microtest: impossible d'ecrire %s\n", path);
        return;
    }
    std::fprintf(file, "exigence;suite;cas;fichier;ligne\n");
    for (std::size_t i = 0U; i < registry.size(); ++i) {
        const microtest::TestCase& tc = registry.at(i);
        if (tc.requirements == nullptr) {
            std::fprintf(file, "(AUCUNE);%s;%s;%s;%d\n", tc.suite, tc.name, tc.file, tc.line);
        } else {
            for_each_requirement(tc.requirements, [&](const char* req) {
                std::fprintf(file, "%s;%s;%s;%s;%d\n", req, tc.suite, tc.name, tc.file, tc.line);
            });
        }
    }
    std::fclose(file);
    std::printf("microtest: tracabilite exportee vers %s\n", path);
}

}  // namespace

namespace microtest {

int run_all(int argc, char** argv) noexcept {
    const Registry& registry = Registry::instance();

    const char* filter = nullptr;
    const char* csv_path = nullptr;
    bool list_only = false;
    bool show_requirements = false;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        if (starts_with(argv[i], "--filter=")) {
            filter = argv[i] + std::strlen("--filter=");
        } else if (starts_with(argv[i], "--csv=")) {
            csv_path = argv[i] + std::strlen("--csv=");
        } else if (std::strcmp(argv[i], "--list") == 0) {
            list_only = true;
        } else if (std::strcmp(argv[i], "--req") == 0) {
            show_requirements = true;
        } else if (std::strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else {
            std::printf("microtest: option inconnue [%s]\n", argv[i]);
            return 2;
        }
    }

    if (registry.overflow()) {
        std::printf("microtest: ERREUR, le registre statique a deborde (kMaxTests=%zu).\n",
                    kMaxTests);
        return 3;
    }

    if (list_only) {
        for (std::size_t i = 0U; i < registry.size(); ++i) {
            const TestCase& tc = registry.at(i);
            std::printf("%s.%s  [%s]\n", tc.suite, tc.name,
                        (tc.requirements != nullptr) ? tc.requirements : "non trace");
        }
        return 0;
    }

    std::printf("microtest : %zu cas de test enregistres\n", registry.size());
    std::printf("-------------------------------------------------------------\n");

    unsigned executed = 0U;
    unsigned failed_cases = 0U;
    detail::RunState& st = detail::state();

    for (std::size_t i = 0U; i < registry.size(); ++i) {
        const TestCase& tc = registry.at(i);
        char name[192];
        full_name(name, sizeof(name), tc);

        if ((filter != nullptr) && !contains(name, filter)) {
            continue;
        }

        st.current = &tc;
        st.current_failures = 0U;
        tc.fn();
        ++executed;

        if (st.current_failures != 0U) {
            ++failed_cases;
            std::printf("[ECHEC] %s\n", name);
        } else if (verbose) {
            std::printf("[  OK  ] %s\n", name);
        }
        st.current = nullptr;
    }

    std::printf("-------------------------------------------------------------\n");
    std::printf("Executes : %u   Reussis : %u   Echoues : %u   Verifications : %u\n", executed,
                executed - failed_cases, failed_cases, st.total_checks);

    if (show_requirements) {
        print_requirement_matrix(registry);
    }
    if (csv_path != nullptr) {
        export_csv(registry, csv_path);
    }

    return (failed_cases == 0U) ? 0 : 1;
}

}  // namespace microtest

int main(int argc, char** argv) {
    return microtest::run_all(argc, argv);
}
