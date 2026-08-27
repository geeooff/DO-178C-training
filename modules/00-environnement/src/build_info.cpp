// =============================================================================
//  Module 00 -- implementation de build_info.
//
//  Ce fichier .cpp est une UNITE DE TRADUCTION (translation unit). Le
//  compilateur le traite isolement, sans rien savoir des autres. C'est la
//  difference majeure avec C# ou le compilateur voit tout l'assembly d'un coup.
//
//  Consequence pratique : si vous appelez ici une fonction definie ailleurs,
//  le compilateur exige d'en connaitre la DECLARATION (via #include), et c'est
//  l'EDITEUR DE LIENS qui, plus tard, ira chercher la DEFINITION. Les erreurs
//  "unresolved external symbol" (LNK2019) viennent de la : le compilateur etait
//  content, l'editeur de liens n'a rien trouve.
// =============================================================================
#include "mod00/build_info.hpp"

namespace mod00 {
namespace {

// `namespace { ... }` = liaison INTERNE : ce symbole n'existe que dans cette
// unite de traduction. Equivalent conceptuel de `private` au niveau fichier,
// et remplacant moderne du `static` global herite du C.
constexpr avio::u32 kUnknownVersion = 0U;

}  // namespace

BuildInfo current_build() noexcept {
    BuildInfo info{};

#if defined(_MSC_VER)
    info.compiler = "MSVC";
    // _MSC_VER : 1951 -> Visual Studio 2026 (toolset 14.51)
    info.compiler_major = static_cast<avio::u32>(_MSC_VER / 100);
    info.compiler_minor = static_cast<avio::u32>(_MSC_VER % 100);
#elif defined(__clang__)
    info.compiler = "Clang";
    info.compiler_major = static_cast<avio::u32>(__clang_major__);
    info.compiler_minor = static_cast<avio::u32>(__clang_minor__);
#elif defined(__GNUC__)
    info.compiler = "GCC";
    info.compiler_major = static_cast<avio::u32>(__GNUC__);
    info.compiler_minor = static_cast<avio::u32>(__GNUC_MINOR__);
#else
    info.compiler = "inconnu";
    info.compiler_major = kUnknownVersion;
    info.compiler_minor = kUnknownVersion;
#endif

    info.cpp_standard = static_cast<avio::i64>(__cplusplus);
    info.pointer_bits = static_cast<avio::u32>(sizeof(void*) * 8U);
    info.little_endian = is_little_endian();
    return info;
}

const char* cpp_standard_name(avio::i64 value) noexcept {
    // Attention : sans /Zc:__cplusplus, MSVC renvoie 199711L quelle que soit la
    // norme demandee. C'est pourquoi cette option est activee dans
    // cmake/TrainingHelpers.cmake. Un detail comme celui-la doit figurer dans
    // le SECI : il change le comportement du code compile.
    if (value >= 202302L) {
        return "C++23";
    }
    if (value >= 202002L) {
        return "C++20";
    }
    if (value >= 201703L) {
        return "C++17";
    }
    if (value >= 201402L) {
        return "C++14";
    }
    if (value >= 201103L) {
        return "C++11";
    }
    return "C++98/03 (ou __cplusplus non conforme)";
}

bool is_little_endian() noexcept {
    // On ecrit un entier 32 bits et on regarde quel octet arrive en premier.
    // L'union est ici le moyen le plus lisible ; noter que MISRA restreint
    // fortement l'usage des unions (type punning). En production on
    // documenterait une deviation, ou on utiliserait std::memcpy.
    const avio::u32 probe = 0x01020304U;
    const avio::u8 first = *reinterpret_cast<const avio::u8*>(&probe);
    return first == 0x04U;
}

}  // namespace mod00
