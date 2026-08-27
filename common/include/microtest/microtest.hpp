// =============================================================================
//  microtest -- micro framework de test unitaire pour la formation DO-178C.
//
//  POURQUOI NE PAS UTILISER GoogleTest / Catch2 ?
//  ----------------------------------------------
//  Dans un projet certifie DO-178C, tout outil dont le RESULTAT est utilise
//  pour eliminer, reduire ou automatiser une activite de verification doit
//  etre QUALIFIE (DO-178C 12.2, et DO-330 pour le detail). Un framework de
//  test est un outil de verification : sa defaillance pourrait laisser passer
//  un faux positif. Il releve typiquement du TQL-5 (Tool Qualification Level 5).
//
//  Qualifier GoogleTest (des dizaines de milliers de lignes, allocation
//  dynamique, exceptions, RTTI) est un chantier enorme. Beaucoup d'equipes
//  avioniques ecrivent donc leur propre harnais, volontairement minuscule et
//  auditable. microtest fait ~300 lignes, n'alloue RIEN dynamiquement,
//  n'utilise ni exception ni RTTI : c'est exactement l'esprit du domaine.
//
//  Contraintes assumees (elles iraient dans les "Tool Operational Requirements")
//    * nombre maximal de cas de test : kMaxTests, verifie a l'enregistrement ;
//    * REQUIRE_* effectue un `return` : utilisable uniquement dans le corps
//      d'un TEST, pas dans une fonction auxiliaire ;
//    * pas de parallelisme, pas de fixtures automatiques.
// =============================================================================
#ifndef MICROTEST_MICROTEST_HPP
#define MICROTEST_MICROTEST_HPP

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace microtest {

/// Nombre maximal de cas de test enregistrables (allocation statique).
constexpr std::size_t kMaxTests = 512U;

/// Description immuable d'un cas de test.
struct TestCase {
    const char* suite;         ///< nom de la suite
    const char* name;          ///< nom du cas
    const char* requirements;  ///< "REQ-A,REQ-B" ou nullptr : lien de tracabilite
    const char* file;          ///< fichier source (preuve d'origine)
    int line;                  ///< ligne de declaration
    void (*fn)();              ///< corps du test
};

namespace detail {

/// Etat global de l'execution. Une seule instance, allouee statiquement.
struct RunState {
    const TestCase* current = nullptr;
    unsigned current_failures = 0U;
    unsigned total_checks = 0U;
    unsigned total_failures = 0U;
};

inline RunState& state() noexcept {
    static RunState s;
    return s;
}

inline void report(const char* file, int line, const char* text) noexcept {
    const RunState& s = state();
    const char* suite = (s.current != nullptr) ? s.current->suite : "?";
    const char* name = (s.current != nullptr) ? s.current->name : "?";
    std::printf("      ECHEC  %s.%s\n", suite, name);
    std::printf("             %s(%d)\n", file, line);
    std::printf("             %s\n", text);
}

inline void count_failure() noexcept {
    state().current_failures += 1U;
    state().total_failures += 1U;
}

/// Formatage minimal d'une valeur dans un tampon de pile (aucune allocation).
template <typename T>
void format_value(char* buffer, std::size_t size, const T& value) noexcept {
    if constexpr (std::is_same_v<T, bool>) {
        std::snprintf(buffer, size, "%s", value ? "true" : "false");
    } else if constexpr (std::is_enum_v<T>) {
        std::snprintf(buffer, size, "%lld",
                      static_cast<long long>(static_cast<std::underlying_type_t<T>>(value)));
    } else if constexpr (std::is_floating_point_v<T>) {
        std::snprintf(buffer, size, "%.9g", static_cast<double>(value));
    } else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
        std::snprintf(buffer, size, "%lld", static_cast<long long>(value));
    } else if constexpr (std::is_integral_v<T>) {
        std::snprintf(buffer, size, "%llu", static_cast<unsigned long long>(value));
    } else if constexpr (std::is_pointer_v<T>) {
        std::snprintf(buffer, size, "%p", static_cast<const void*>(value));
    } else {
        std::snprintf(buffer, size, "<valeur non affichable>");
    }
}

inline bool check_bool(bool condition, const char* expression, const char* file,
                       int line) noexcept {
    state().total_checks += 1U;
    if (!condition) {
        char text[256];
        std::snprintf(text, sizeof(text), "attendu vrai : %s", expression);
        report(file, line, text);
        count_failure();
    }
    return condition;
}

inline bool check_false(bool condition, const char* expression, const char* file,
                        int line) noexcept {
    state().total_checks += 1U;
    if (condition) {
        char text[256];
        std::snprintf(text, sizeof(text), "attendu faux : %s", expression);
        report(file, line, text);
        count_failure();
    }
    return !condition;
}

template <typename A, typename B>
bool check_eq(const A& actual, const B& expected, const char* a_txt, const char* b_txt,
              const char* file, int line) noexcept {
    state().total_checks += 1U;
    const bool ok = (actual == expected);
    if (!ok) {
        char va[64];
        char vb[64];
        format_value(va, sizeof(va), actual);
        format_value(vb, sizeof(vb), expected);
        char text[512];
        std::snprintf(text, sizeof(text), "%s == %s  ->  obtenu %s, attendu %s", a_txt, b_txt, va,
                      vb);
        report(file, line, text);
        count_failure();
    }
    return ok;
}

/// Surcharge non generique : deux chaines C se comparent par contenu, pas par
/// adresse. Piege classique pour un developpeur C# ou `==` sur string compare
/// le contenu.
inline bool check_eq(const char* actual, const char* expected, const char* a_txt, const char* b_txt,
                     const char* file, int line) noexcept {
    state().total_checks += 1U;
    const bool ok =
        (actual != nullptr) && (expected != nullptr) && (std::strcmp(actual, expected) == 0);
    if (!ok) {
        char text[512];
        std::snprintf(text, sizeof(text), "%s == %s  ->  obtenu [%s], attendu [%s]", a_txt, b_txt,
                      (actual != nullptr) ? actual : "(null)",
                      (expected != nullptr) ? expected : "(null)");
        report(file, line, text);
        count_failure();
    }
    return ok;
}

inline bool check_near(double actual, double expected, double tolerance, const char* a_txt,
                       const char* b_txt, const char* file, int line) noexcept {
    state().total_checks += 1U;
    const double delta = (actual > expected) ? (actual - expected) : (expected - actual);
    const bool ok = (delta <= tolerance);
    if (!ok) {
        char text[512];
        std::snprintf(text, sizeof(text),
                      "%s ~= %s  ->  obtenu %.9g, attendu %.9g (ecart %.3g > tolerance %.3g)",
                      a_txt, b_txt, actual, expected, delta, tolerance);
        report(file, line, text);
        count_failure();
    }
    return ok;
}

inline bool fail_now(const char* message, const char* file, int line) noexcept {
    state().total_checks += 1U;
    report(file, line, message);
    count_failure();
    return false;
}

}  // namespace detail

/// Registre statique des cas de test (aucune allocation dynamique).
class Registry {
public:
    static Registry& instance() noexcept {
        static Registry registry;
        return registry;
    }

    bool add(const TestCase& test_case) noexcept {
        if (count_ >= kMaxTests) {
            overflow_ = true;
            return false;
        }
        tests_[count_] = test_case;
        count_ += 1U;
        return true;
    }

    std::size_t size() const noexcept { return count_; }
    const TestCase& at(std::size_t index) const noexcept { return tests_[index]; }
    bool overflow() const noexcept { return overflow_; }

private:
    Registry() = default;
    TestCase tests_[kMaxTests] = {};
    std::size_t count_ = 0U;
    bool overflow_ = false;
};

namespace detail {
/// Objet dont la seule raison d'etre est de s'enregistrer pendant
/// l'initialisation statique. C'est l'equivalent C++ de l'attribut [TestMethod]
/// en C#, mais resolu a l'edition de liens plutot que par reflexion.
struct Registrar {
    Registrar(const char* suite, const char* name, const char* reqs, const char* file, int line,
              void (*fn)()) noexcept {
        Registry::instance().add(TestCase{suite, name, reqs, file, line, fn});
    }
};
}  // namespace detail

/// Execute les tests enregistres. Renvoie le nombre d'echecs (0 = succes).
int run_all(int argc, char** argv) noexcept;

}  // namespace microtest

// -----------------------------------------------------------------------------
//  Macros de declaration
// -----------------------------------------------------------------------------
#define MT_CONCAT_IMPL(a, b) a##b
#define MT_CONCAT(a, b) MT_CONCAT_IMPL(a, b)

#define MT_TEST_IMPL(suite_id, test_id, reqs)                                                   \
    static void MT_CONCAT(mt_body_, MT_CONCAT(suite_id, MT_CONCAT(_, test_id)))();              \
    namespace {                                                                                 \
    const ::microtest::detail::Registrar MT_CONCAT(mt_reg_,                                     \
                                                   MT_CONCAT(suite_id, MT_CONCAT(_, test_id)))( \
        #suite_id, #test_id, reqs, __FILE__, __LINE__,                                          \
        &MT_CONCAT(mt_body_, MT_CONCAT(suite_id, MT_CONCAT(_, test_id))));                      \
    }                                                                                           \
    static void MT_CONCAT(mt_body_, MT_CONCAT(suite_id, MT_CONCAT(_, test_id)))()

/// Declare un cas de test SANS lien de tracabilite (a proscrire en DO-178C).
#define TEST(suite_id, test_id) MT_TEST_IMPL(suite_id, test_id, nullptr)

/// Declare un cas de test TRACE vers une ou plusieurs exigences.
/// Exemple : TEST_REQ(Filtre, saturation_haute, "LLR-FLT-004,LLR-FLT-005")
#define TEST_REQ(suite_id, test_id, reqs) MT_TEST_IMPL(suite_id, test_id, reqs)

// -----------------------------------------------------------------------------
//  Macros de verification
//   CHECK_*   : signale l'echec et POURSUIT le test
//   REQUIRE_* : signale l'echec et INTERROMPT le test (evite les crashs en cascade)
// -----------------------------------------------------------------------------
#define CHECK(expr) (void)::microtest::detail::check_bool((expr), #expr, __FILE__, __LINE__)
#define CHECK_FALSE(expr) (void)::microtest::detail::check_false((expr), #expr, __FILE__, __LINE__)
#define CHECK_EQ(a, b) (void)::microtest::detail::check_eq((a), (b), #a, #b, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tol) \
    (void)::microtest::detail::check_near((a), (b), (tol), #a, #b, __FILE__, __LINE__)

#define REQUIRE(expr)                                                              \
    do {                                                                           \
        if (!::microtest::detail::check_bool((expr), #expr, __FILE__, __LINE__)) { \
            return;                                                                \
        }                                                                          \
    } while (false)

#define REQUIRE_EQ(a, b)                                                            \
    do {                                                                            \
        if (!::microtest::detail::check_eq((a), (b), #a, #b, __FILE__, __LINE__)) { \
            return;                                                                 \
        }                                                                           \
    } while (false)

#define FAIL(message) (void)::microtest::detail::fail_now((message), __FILE__, __LINE__)

#endif  // MICROTEST_MICROTEST_HPP
