// =============================================================================
//  Module 00 -- programme de demonstration.
//
//  Lancez-le : build/debug/bin/demo_00-environnement.exe
// =============================================================================
#include "mod00/build_info.hpp"

#include <avio/types.hpp>

#include <cstdio>

namespace {

void print_separator(const char* title) {
    std::printf("\n=== %s ===\n", title);
}

void print_type_sizes() {
    print_separator("Tailles des types sur CETTE cible");
    std::printf("  %-22s %zu octet(s)\n", "bool", sizeof(bool));
    std::printf("  %-22s %zu octet(s)\n", "char", sizeof(char));
    std::printf("  %-22s %zu octet(s)\n", "short", sizeof(short));
    std::printf("  %-22s %zu octet(s)\n", "int", sizeof(int));
    std::printf("  %-22s %zu octet(s)\n", "long", sizeof(long));
    std::printf("  %-22s %zu octet(s)\n", "long long", sizeof(long long));
    std::printf("  %-22s %zu octet(s)\n", "float", sizeof(float));
    std::printf("  %-22s %zu octet(s)\n", "double", sizeof(double));
    std::printf("  %-22s %zu octet(s)\n", "void*", sizeof(void*));
    std::printf("\n  En C#, TOUTES ces tailles sont fixees par la norme.\n");
    std::printf("  En C++, seules celles de <cstdint> le sont : u8/u16/u32/u64.\n");
}

void print_build_info() {
    const mod00::BuildInfo info = mod00::current_build();
    print_separator("Environnement de production (a figer dans le SECI)");
    std::printf("  Compilateur   : %s %u.%u\n", info.compiler, info.compiler_major,
                info.compiler_minor);
    std::printf("  Norme C++     : %s (__cplusplus = %lld)\n",
                mod00::cpp_standard_name(info.cpp_standard),
                static_cast<long long>(info.cpp_standard));
    std::printf("  Pointeurs     : %u bits\n", info.pointer_bits);
    std::printf("  Boutisme      : %s\n", info.little_endian ? "petit (little endian)"
                                                             : "grand (big endian)");
    std::printf("  Date de build : %s %s\n", __DATE__, __TIME__);
    std::printf("\n  __DATE__/__TIME__ rendent le binaire NON reproductible bit a bit.\n");
    std::printf("  En certification on prefere une empreinte du depot (commit Git).\n");
}

void print_compilation_model() {
    print_separator("Le modele de compilation, vu depuis C#");
    std::printf("  C#  : .cs --> compilateur --> IL dans un assembly --> JIT --> code machine\n");
    std::printf("        Les metadonnees decrivent tout ; la reflexion est possible.\n\n");
    std::printf("  C++ : .cpp --[preprocesseur]--> unite de traduction\n");
    std::printf("            --[compilateur]--> .obj (code machine + symboles)\n");
    std::printf("            --[editeur de liens]--> .exe / .lib\n");
    std::printf("        Aucune metadonnee : ce qui n'est pas dans un .hpp est invisible.\n");
    std::printf("        C'est aussi pourquoi l'analyse statique C++ est plus difficile,\n");
    std::printf("        et pourquoi la DO-178C insiste tant sur les revues et la tracabilite.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 00 : environnement, chaine de compilation, DO-178C #\n");
    std::printf("#############################################################\n");

    print_build_info();
    print_type_sizes();
    print_compilation_model();

    std::printf("\nModule 00 termine. Lancez maintenant les tests :\n");
    std::printf("  ctest --preset debug -R 00-environnement --output-on-failure\n");
    return 0;
}
