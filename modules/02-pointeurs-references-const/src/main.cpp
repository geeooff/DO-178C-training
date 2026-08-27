// =============================================================================
//  Module 02 -- demonstration : pointeurs, references, const.
// =============================================================================
#include <avio/span.hpp>
#include <avio/types.hpp>
#include <cstdio>

#include "mod02/buffers.hpp"

using avio::i32;
using avio::u8;
using avio::usize;

namespace {

void title(const char* text) {
    std::printf("\n=== %s ===\n", text);
}

// -----------------------------------------------------------------------------
void reference_vs_pointer() {
    title("Reference ou pointeur : comment choisir");

    i32 a = 1;
    i32 b = 2;
    std::printf("  avant  : a=%d b=%d\n", a, b);
    mod02::swap_values(a, b);
    std::printf("  apres  : a=%d b=%d   (references : les objets existent forcement)\n", a, b);

    i32 counter = 41;
    const bool ok = mod02::increment_if_valid(&counter);
    std::printf("  increment_if_valid(&compteur) -> %s, compteur=%d\n", ok ? "true" : "false",
                counter);
    std::printf("  increment_if_valid(nullptr)   -> %s\n",
                mod02::increment_if_valid(nullptr) ? "true" : "false");

    std::printf("\n  REGLE : reference par defaut. Pointeur SEULEMENT si\n");
    std::printf("  l'absence de valeur a un sens metier. Un pointeur dans une\n");
    std::printf("  signature, c'est un test de nullite a ecrire, donc une branche\n");
    std::printf("  a couvrir, donc un cas de test a justifier.\n");
}

// -----------------------------------------------------------------------------
void array_decay() {
    title("Le tableau qui perd sa taille (array decay)");

    const i32 array[5] = {1, 2, 3, 4, 5};
    const i32* pointer = array;  // conversion implicite : on perd la taille

    std::printf("  sizeof(tableau)  = %zu octets  (le tableau connait sa taille)\n", sizeof(array));
    std::printf("  sizeof(pointeur) = %zu octets  (juste une adresse !)\n", sizeof(pointer));
    std::printf("\n  Des qu'un tableau est passe a une fonction, il devient un pointeur\n");
    std::printf("  et la taille est PERDUE. `void f(int t[10])` accepte un tableau\n");
    std::printf("  de 3 elements sans broncher : le 10 est purement decoratif.\n");
    std::printf("\n  Solution : avio::Span<T>, qui transporte (pointeur, taille) ensemble.\n");

    const avio::Span<const i32> view = avio::make_const_span(array);
    std::printf("  vue.size() = %zu  -- la taille voyage avec la donnee.\n", view.size());
}

// -----------------------------------------------------------------------------
void const_correctness() {
    title("Les quatre const (se lisent de droite a gauche)");

    i32 x = 1;
    i32 y = 2;

    i32* p1 = &x;              // pointeur modifiable -> entier modifiable
    const i32* p2 = &x;        // pointeur modifiable -> entier CONSTANT
    i32* const p3 = &x;        // pointeur CONSTANT   -> entier modifiable
    const i32* const p4 = &x;  // tout constant

    *p1 = 10;  // OK
    p1 = &y;   // OK
    // *p2 = 10;  // refuse : on ne modifie pas a travers p2
    p2 = &y;   // OK
    *p3 = 20;  // OK
    // p3 = &y;   // refuse : p3 ne peut pas changer de cible
    avio::unused(p2, p4);

    std::printf("  x vaut %d apres modifications via p1 puis p3\n", x);
    std::printf("\n  En C#, `readonly List<int> l` interdit de REMPLACER l,\n");
    std::printf("  mais pas de faire l.Add(...). `const` en C++ distingue les deux,\n");
    std::printf("  et le compilateur l'impose. C'est un outil de conception, pas\n");
    std::printf("  un commentaire : `const` dans une signature est un CONTRAT.\n");
}

// -----------------------------------------------------------------------------
void bounded_buffers() {
    title("Copie bornee : la fin des debordements de tampon");

    u8 source[6] = {1U, 2U, 3U, 4U, 5U, 6U};
    u8 small[3] = {0U, 0U, 0U};

    const usize copies = mod02::copy_bounded(avio::make_const_span(source), avio::make_span(small));

    std::printf("  source de %zu octets -> destination de %zu octets\n", sizeof(source),
                sizeof(small));
    std::printf("  octets copies : %zu  (aucun debordement possible)\n", copies);
    std::printf("  destination   : %u %u %u\n", small[0], small[1], small[2]);
    std::printf("\n  Comparez avec memcpy(petit, source, sizeof(source)) : compile,\n");
    std::printf("  s'execute, corrompt la pile, et plante 200 ms plus tard ailleurs.\n");

    const avio::u16 sum = mod02::checksum16(avio::make_const_span(source));
    std::printf("\n  checksum16(source) = 0x%04X\n", sum);
}

// -----------------------------------------------------------------------------
void circular_log() {
    title("MeasurementLog : capacite fixe, ecrasement du plus ancien");

    mod02::MeasurementLog log;
    for (i32 i = 1; i <= 11; ++i) {
        log.push(i * 10);
    }

    std::printf("  capacite      : %zu\n", mod02::MeasurementLog::kCapacity);
    std::printf("  taille        : %zu\n", log.size());
    std::printf("  a deborde     : %s\n", log.has_overflowed() ? "oui" : "non");
    std::printf("  contenu (du plus ancien au plus recent) :");
    for (usize index = 0U; index < log.size(); ++index) {
        i32 value = 0;
        if (log.at(index, value)) {
            std::printf(" %d", value);
        }
    }
    std::printf("\n");
    std::printf("\n  Capacite FIXE, connue a la compilation : pas d'allocation,\n");
    std::printf("  pas de fragmentation, occupation memoire calculable. C'est le\n");
    std::printf("  seul modele acceptable dans un calculateur certifie (module 08).\n");
}

// -----------------------------------------------------------------------------
void lifetime_pitfalls() {
    title("Duree de vie : ce que le GC faisait pour vous");

    std::printf("  Les trois fautes que le compilateur ne detecte PAS :\n\n");
    std::printf("  1. Renvoyer l'adresse d'une variable locale\n");
    std::printf("       const int* f() { int x = 42; return &x; }   // x meurt\n\n");
    std::printf("  2. Conserver une Span vers un tampon detruit\n");
    std::printf("       Span<int> s;\n");
    std::printf("       { int t[4] = {}; s = make_span(t); }        // t meurt\n");
    std::printf("       s[0] = 1;                                   // corruption\n\n");
    std::printf("  3. Garder un pointeur vers un element d'un conteneur qui grossit\n\n");
    std::printf("  En C#, tant qu'une reference existe, l'objet vit : le GC s'en\n");
    std::printf("  charge. En C++, la duree de vie est une propriete que VOUS devez\n");
    std::printf("  demontrer. C'est un point de revue de code obligatoire, et l'une\n");
    std::printf("  des raisons pour lesquelles MISRA limite tant les pointeurs.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 02 : pointeurs, references, const-correctness      #\n");
    std::printf("#############################################################\n");

    reference_vs_pointer();
    array_decay();
    const_correctness();
    bounded_buffers();
    circular_log();
    lifetime_pitfalls();

    std::printf("\nModule 02 termine.\n");
    return 0;
}
