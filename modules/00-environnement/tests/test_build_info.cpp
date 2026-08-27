// =============================================================================
//  Module 00 -- tests unitaires.
//
//  Chaque test est TRACE vers une exigence (voir README.md, section
//  "Exigences du module"). C'est la regle d'or DO-178C : pas de test sans
//  exigence, pas d'exigence sans test.
// =============================================================================
#include <avio/types.hpp>
#include <microtest/microtest.hpp>

#include "mod00/build_info.hpp"

TEST_REQ(BuildInfo, compiler_identifies_it, "LLR-M00-001") {
    const mod00::BuildInfo info = mod00::current_build();
    REQUIRE(info.compiler != nullptr);
    CHECK_FALSE(mod00::current_build().compiler[0] == '\0');
}

TEST_REQ(BuildInfo, cpp17_standard_minimum, "LLR-M00-002") {
    // Si ce test echoue, c'est que /Zc:__cplusplus ou /std:c++17 manque :
    // le code compile ne serait alors PAS celui que l'on croit verifier.
    const mod00::BuildInfo info = mod00::current_build();
    CHECK(info.cpp_standard >= 201703L);
}

TEST_REQ(BuildInfo, standard_name_by_range, "LLR-M00-003") {
    CHECK_EQ(mod00::cpp_standard_name(201703L), "C++17");
    CHECK_EQ(mod00::cpp_standard_name(202002L), "C++20");
    CHECK_EQ(mod00::cpp_standard_name(201402L), "C++14");
    CHECK_EQ(mod00::cpp_standard_name(201103L), "C++11");
}

TEST_REQ(BuildInfo, robustness_value_out_of_domain, "LLR-M00-003") {
    // Cas de ROBUSTESSE : entree invalide (negative, jamais produite par un
    // compilateur). La fonction doit rester deterministe, pas planter.
    CHECK_EQ(mod00::cpp_standard_name(-1), "C++98/03 (ou __cplusplus non conforme)");
    CHECK_EQ(mod00::cpp_standard_name(0), "C++98/03 (ou __cplusplus non conforme)");
}

TEST_REQ(BuildInfo, consistent_pointer_width, "LLR-M00-004") {
    const mod00::BuildInfo info = mod00::current_build();
    CHECK(((info.pointer_bits == 32U) || (info.pointer_bits == 64U)));
    CHECK_EQ(info.pointer_bits, static_cast<avio::u32>(sizeof(void*) * 8U));
}

TEST_REQ(BuildInfo, consistent_endianness, "LLR-M00-005") {
    const mod00::BuildInfo info = mod00::current_build();
    CHECK_EQ(info.little_endian, mod00::is_little_endian());
    // x86/x64 est petit-boutiste. Beaucoup de calculateurs avioniques
    // (PowerPC, SPARC) sont grand-boutistes : ne jamais coder en dur.
    CHECK(info.little_endian);
}
