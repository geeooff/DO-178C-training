// =============================================================================
//  Module 06 -- demonstration : templates, constexpr, CRTP.
// =============================================================================
#include "mod06/compile_time.hpp"
#include "mod06/ring_buffer.hpp"
#include "mod06/static_polymorphism.hpp"

#include <avio/span.hpp>
#include <avio/types.hpp>

#include <cstdio>

using avio::f32;
using avio::i32;
using avio::u8;
using avio::usize;

namespace {

void titre(const char* texte) {
    std::printf("\n=== %s ===\n", texte);
}

// -----------------------------------------------------------------------------
void un_patron_plusieurs_types() {
    titre("Un seul code source, plusieurs codes executables");

    mod06::RingBuffer<i32, 4U> entiers;
    mod06::RingBuffer<f32, 8U> flottants;

    for (i32 index = 1; index <= 6; ++index) {
        (void)entiers.push(index * 10);
    }
    for (i32 index = 0; index < 4; ++index) {
        (void)flottants.push(static_cast<f32>(index) * 0.5F);
    }

    std::printf("  RingBuffer<i32, 4>  : taille=%zu, ecrasements=%u, moyenne=%d\n",
                entiers.size(), entiers.overwrite_count(), mod06::average(entiers));
    std::printf("  RingBuffer<f32, 8>  : taille=%zu, ecrasements=%u, moyenne=%.4f\n",
                flottants.size(), flottants.overwrite_count(),
                static_cast<double>(mod06::average(flottants)));

    std::printf("\n  sizeof(RingBuffer<i32, 4>)  = %3zu octets\n",
                sizeof(mod06::RingBuffer<i32, 4U>));
    std::printf("  sizeof(RingBuffer<f32, 8>)  = %3zu octets\n",
                sizeof(mod06::RingBuffer<f32, 8U>));
    std::printf("  sizeof(RingBuffer<u8, 16>)  = %3zu octets\n",
                sizeof(mod06::RingBuffer<u8, 16U>));

    std::printf("\n  Trois TYPES differents, trois codes machine differents.\n");
    std::printf("  DO-332 : la couverture structurelle doit etre obtenue POUR\n");
    std::printf("  CHACUNE des instanciations embarquees. Couvrir la version i32\n");
    std::printf("  ne dit rien de la version f32.\n");
}

// -----------------------------------------------------------------------------
void calcul_a_la_compilation() {
    titre("constexpr : le calcul deplace vers le compilateur");

    std::printf("  Table CRC-8 (256 entrees) calculee A LA COMPILATION :\n    ");
    for (usize index = 0U; index < 16U; ++index) {
        std::printf("%02X ", mod06::kCrc8Table.values[index]);
    }
    std::printf("...\n");

    const u8 message[9] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U};
    std::printf("\n  crc8(\"123456789\") = 0x%02X   (vecteur de test CRC-8/SMBUS)\n",
                mod06::crc8(avio::make_const_span(message)));

    std::printf("\n  ipow(2, 16)  = %llu\n",
                static_cast<unsigned long long>(mod06::ipow(2U, 16U)));
    std::printf("  popcount(0xF0F0) = %u\n", mod06::popcount(0xF0F0U));

    std::printf("\n  Ce que cela apporte en certification :\n");
    std::printf("    * aucun code d'initialisation dans le binaire\n");
    std::printf("      -> rien a tracer, rien a tester, rien a couvrir\n");
    std::printf("    * la table est en ROM : impossible de la corrompre en RAM\n");
    std::printf("    * une erreur de calcul = une erreur de COMPILATION\n");
    std::printf("      (voir les static_assert dans compile_time.hpp)\n");
    std::printf("    * zero cycle a l'execution : le WCET n'en souffre pas\n");
}

// -----------------------------------------------------------------------------
void dynamique_contre_statique() {
    titre("Polymorphisme statique (CRTP) contre dynamique (module 05)");

    const mod06::StaticPressureSensor pression;
    const mod06::StaticTemperatureSensor temperature;
    const i32 echantillons[4] = {0, 1365, 2730, 4095};

    std::printf("  moyenne pression    : %8.2f hPa\n",
                static_cast<double>(mod06::lire_moyenne(pression, echantillons, 4U)));
    std::printf("  moyenne temperature : %8.2f degres C\n",
                static_cast<double>(mod06::lire_moyenne(temperature, echantillons, 4U)));

    std::printf("\n  %-34s %s\n", "", "taille d'un objet");
    std::printf("  %-34s %zu octet(s)\n", "mod06::StaticPressureSensor (CRTP)",
                sizeof(mod06::StaticPressureSensor));
    std::printf("  %-34s %zu octet(s)  <- pointeur de vtable\n",
                "mod05::PressureSensor (virtuel)", sizeof(void*));

    std::printf("\n  Meme factorisation de code, meme lisibilite, mais :\n");
    std::printf("    * aucune vtable, aucune indirection ;\n");
    std::printf("    * les appels sont inlinables -> WCET exact et plus faible ;\n");
    std::printf("    * en contrepartie, IMPOSSIBLE de faire un tableau heterogene :\n");
    std::printf("      les types doivent etre connus a la compilation.\n");
    std::printf("\n  Dans un calculateur certifie, les types SONT connus a la\n");
    std::printf("  compilation dans l'immense majorite des cas.\n");
}

// -----------------------------------------------------------------------------
void pieges_des_templates() {
    titre("Les pieges des templates");

    std::printf("  1. GONFLEMENT DU CODE (code bloat)\n");
    std::printf("     Chaque instanciation duplique le code. Dix instanciations\n");
    std::printf("     d'un conteneur de 2 ko, c'est 20 ko de Flash. Sur une cible\n");
    std::printf("     qui en a 512 ko, cela se planifie.\n\n");
    std::printf("  2. MESSAGES D'ERREUR ILLISIBLES\n");
    std::printf("     La validite n'est verifiee qu'a l'INSTANCIATION. Parade :\n");
    std::printf("     des static_assert explicites en tete de template, comme dans\n");
    std::printf("     RingBuffer. Le message devient celui que VOUS avez ecrit.\n\n");
    std::printf("  3. COUVERTURE PAR INSTANCIATION (DO-332)\n");
    std::printf("     C'est l'exigence la plus souvent oubliee. Elle impose de\n");
    std::printf("     LISTER les instanciations embarquees dans la conception --\n");
    std::printf("     d'ou les instanciations explicites en bas de compile_time.cpp.\n\n");
    std::printf("  4. TEMPS DE COMPILATION\n");
    std::printf("     Anecdotique ici, majeur sur un projet reel.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 06 : templates, constexpr, polymorphisme statique  #\n");
    std::printf("#############################################################\n");

    un_patron_plusieurs_types();
    calcul_a_la_compilation();
    dynamique_contre_statique();
    pieges_des_templates();

    std::printf("\nModule 06 termine.\n");
    return 0;
}
