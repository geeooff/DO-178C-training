// =============================================================================
//  Auto-verification du harnais microtest et des briques avio.
//
//  DO-330 : un outil de verification doit etre verifie. Ces tests constituent
//  la preuve minimale que les macros CHECK_*/REQUIRE_* et Span se comportent
//  comme specifie dans les "Tool Operational Requirements".
// =============================================================================
#include <microtest/microtest.hpp>

#include <avio/assert.hpp>
#include <avio/span.hpp>
#include <avio/types.hpp>

namespace {

avio::u32 g_handler_calls = 0U;

void silent_handler(const char* condition, const char* file, avio::i32 line) noexcept {
    avio::unused(condition, file, line);
    g_handler_calls += 1U;
}

}  // namespace

TEST_REQ(Microtest, comparaisons_entieres, "TOOL-MT-001") {
    CHECK_EQ(2 + 2, 4);
    CHECK(1 < 2);
    CHECK_FALSE(2 < 1);
}

TEST_REQ(Microtest, comparaison_chaines_par_contenu, "TOOL-MT-002") {
    // Deux pointeurs differents, meme contenu : la surcharge const char*
    // doit comparer le CONTENU (piege classique venant de C#).
    char buffer[8] = {'v', 'o', 'l', '\0'};
    const char* literal = "vol";
    CHECK_EQ(static_cast<const char*>(buffer), literal);
}

TEST_REQ(Microtest, tolerance_flottante, "TOOL-MT-003") {
    const double a = 0.1 + 0.2;  // != 0.3 en IEEE-754 !
    CHECK_NEAR(a, 0.3, 1e-12);
}

TEST_REQ(Span, taille_et_acces, "TOOL-SPAN-001") {
    avio::i32 values[4] = {10, 20, 30, 40};
    const avio::Span<avio::i32> span = avio::make_span(values);

    REQUIRE_EQ(span.size(), static_cast<avio::usize>(4));
    CHECK_EQ(span[0], 10);
    CHECK_EQ(span[3], 40);
    CHECK_FALSE(span.empty());
}

TEST_REQ(Span, sous_vue_valide_et_invalide, "TOOL-SPAN-002") {
    avio::i32 values[5] = {1, 2, 3, 4, 5};
    const avio::Span<avio::i32> span = avio::make_span(values);

    const avio::Span<avio::i32> middle = span.subspan(1U, 3U);
    REQUIRE_EQ(middle.size(), static_cast<avio::usize>(3));
    CHECK_EQ(middle[0], 2);
    CHECK_EQ(middle[2], 4);

    // Robustesse : une sous-vue hors bornes renvoie une vue vide, pas un
    // comportement indefini.
    const avio::Span<avio::i32> invalid = span.subspan(3U, 10U);
    CHECK(invalid.empty());
}

TEST_REQ(Assert, gestionnaire_appele_sur_violation, "TOOL-ASSERT-001") {
    g_handler_calls = 0U;
    avio::reset_fault_count();
    const avio::FaultHandler previous = avio::set_fault_handler(&silent_handler);

    avio::i32 values[2] = {7, 8};
    const avio::Span<avio::i32> span = avio::make_span(values);
    (void)span.at(99U);  // acces hors bornes volontaire

    avio::set_fault_handler(previous);
    CHECK_EQ(g_handler_calls, 1U);
    CHECK_EQ(avio::fault_count(), 1U);
}

TEST_REQ(Assert, aucun_appel_quand_condition_vraie, "TOOL-ASSERT-002") {
    g_handler_calls = 0U;
    const avio::FaultHandler previous = avio::set_fault_handler(&silent_handler);

    avio::i32 values[2] = {7, 8};
    const avio::Span<avio::i32> span = avio::make_span(values);
    CHECK_EQ(span.at(1U), 8);

    avio::set_fault_handler(previous);
    CHECK_EQ(g_handler_calls, 0U);
}
