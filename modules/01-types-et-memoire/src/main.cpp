// =============================================================================
//  Module 01 -- demonstration : les pieges des types et de la memoire.
// =============================================================================
#include <avio/types.hpp>
#include <cstdio>
#include <limits>

#include "mod01/safe_arith.hpp"

using avio::i16;
using avio::i32;
using avio::u32;
using avio::u8;

namespace {

void titre(const char* texte) {
    std::printf("\n=== %s ===\n", texte);
}

// -----------------------------------------------------------------------------
void promotion_entiere() {
    titre("Promotion entiere : a + b n'a pas le type de a ni de b");

    const u8 a = 200U;
    const u8 b = 100U;

    // a et b sont PROMUS en `int` avant l'addition. Le resultat est 300,
    // pas 44. Beaucoup de developpeurs attendent une arithmetique 8 bits.
    const int somme_promue = a + b;
    const u8 somme_tronquee = static_cast<u8>(a + b);

    std::printf("  u8(200) + u8(100) evalue en int   -> %d\n", somme_promue);
    std::printf("  ... puis tronque en u8            -> %u\n", somme_tronquee);
    std::printf("  En C#, byte+byte donne int AUSSI, mais la conversion inverse\n");
    std::printf("  exige un cast explicite : le compilateur vous protege.\n");
    std::printf("  En C++, `u8 s = a + b;` compile sans broncher (avec /W4 il previent).\n");
}

// -----------------------------------------------------------------------------
void comparaison_signee_non_signee() {
    titre("Comparaison signe / non signe");

    const int moins_un = -1;
    const unsigned int un = 1U;

    // SUPPRESSION PORTABLE D UN AVERTISSEMENT.
    //
    // Les trois grands compilateurs signalent cette comparaison, et ils ont
    // raison. Elle est VOULUE ici : c'est l'objet meme de la demonstration.
    // Il faut donc la taire sur les trois, et dire pourquoi.
    //
    // Notez que chaque compilateur a sa propre syntaxe et qu'il n'existe
    // aucun mecanisme standard. C'est exactement le genre de detail qui rend
    // une base de code non portable si l'on ne s'en occupe pas des le premier
    // jour -- et c'est pourquoi ce depot se compile sur les trois.
// ORDRE DES TESTS : on interroge __GNUC__ et __clang__ AVANT _MSC_VER.
// Raison : clang-cl (Clang en mode compatible Visual Studio) definit LES DEUX.
// Tester _MSC_VER en premier lui ferait prendre la branche MSVC, dont il ne
// connait pas les numeros d'avertissement -- et la suppression serait sans
// effet. Piege classique du code portable.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4018 4389)
#endif
    // Regle des conversions arithmetiques usuelles : `moins_un` est converti
    // en unsigned, donc vaut 4294967295. La comparaison est donc FAUSSE.
    const bool surprise = (moins_un < un);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

    std::printf("  (-1 < 1u) vaut %s  <-- ce n'est pas une faute de frappe\n",
                surprise ? "true" : "false");
    std::printf("  Cause : -1 devient %u apres conversion vers unsigned.\n",
                static_cast<unsigned int>(moins_un));
    std::printf("  MISRA C++ interdit ce melange. clang-tidy et /W4 le signalent.\n");
    std::printf("  Corollaire : ne JAMAIS ecrire `for (u32 i = n - 1; i >= 0; --i)`\n");
    std::printf("               -- la condition est toujours vraie : boucle infinie.\n");
}

// -----------------------------------------------------------------------------
void debordement() {
    titre("Debordement : defini pour le non signe, INDEFINI pour le signe");

    u8 compteur = 255U;
    compteur = static_cast<u8>(compteur + 1U);  // arithmetique modulo 256 : defini
    std::printf("  u8 255 + 1 = %u  (modulo 2^8, comportement DEFINI par la norme)\n", compteur);

    std::printf("  i32 2147483647 + 1 = COMPORTEMENT INDEFINI.\n");
    std::printf("  Le compilateur a le droit de supposer que cela n'arrive jamais\n");
    std::printf("  et de SUPPRIMER votre test `if (x + 1 < x)`. C'est arrive en vrai.\n");

    std::printf("\n  Avec les briques du module :\n");
    std::printf("    saturating_add(32767, 1)   = %d\n",
                mod01::saturating_add(mod01::kI16Max, i16{1}));
    i32 resultat = 0;
    const bool ok = mod01::checked_add(std::numeric_limits<i32>::max(), 1, resultat);
    std::printf("    checked_add(INT_MAX, 1)    = %s (resultat neutralise a %d)\n",
                ok ? "true" : "false", resultat);
}

// -----------------------------------------------------------------------------
void initialisation() {
    titre("Initialisation : le defaut n'existe pas");

    // ATTENTION : `int x;` a l'interieur d'une fonction laisse x AVEC UNE
    // VALEUR INDETERMINEE. Le lire est un comportement indefini. On ne le fait
    // donc pas ici -- on montre seulement les formes correctes.
    int zero_initialise{};  // vaut 0
    int explicitement_initialise = 42;
    u32 tableau[4] = {};  // les 4 elements valent 0

    std::printf("  int x{};        -> %d   (initialisation de valeur)\n", zero_initialise);
    std::printf("  int y = 42;     -> %d\n", explicitement_initialise);
    std::printf("  u32 t[4] = {};  -> %u %u %u %u\n", tableau[0], tableau[1], tableau[2],
                tableau[3]);
    std::printf("\n  En C#, tout champ et tout element de tableau est zero-initialise,\n");
    std::printf("  et le compilateur refuse de lire une variable locale non assignee.\n");
    std::printf("  En C++, `int x;` local n'est PAS initialise : lire x est un UB.\n");
    std::printf("  Regle du projet : TOUJOURS initialiser a la declaration.\n");
    std::printf("  (regle clang-tidy `cppcoreguidelines-init-variables`, active ici)\n");
}

// -----------------------------------------------------------------------------
void disposition_memoire() {
    titre("Disposition memoire : taille, alignement, bourrage");

    std::printf("  NaiveFrame   { u8; u32; u8; }  -> sizeof = %zu, alignof = %zu\n",
                sizeof(mod01::NaiveFrame), alignof(mod01::NaiveFrame));
    std::printf("  CompactFrame{ u32; u8; u8; }  -> sizeof = %zu, alignof = %zu\n",
                sizeof(mod01::CompactFrame), alignof(mod01::CompactFrame));
    std::printf("\n  Meme information, 33%% de RAM en moins simplement en reordonnant\n");
    std::printf("  les champs du plus large au plus etroit. Sur un calculateur avec\n");
    std::printf("  2 Mo de RAM et 10000 messages en tampon, cela se voit.\n");
    std::printf("\n  ATTENTION : le bourrage n'est pas initialise. Comparer deux structs\n");
    std::printf("  avec memcmp compare AUSSI le bourrage -> resultat non fiable.\n");
}

// -----------------------------------------------------------------------------
void enumerations() {
    titre("Enumerations fortement typees");

    for (u8 raw = 0U; raw < 6U; ++raw) {
        const mod01::SensorId id = static_cast<mod01::SensorId>(raw);
        std::printf("  valeur brute %u -> %-14s (valide : %s)\n", raw, mod01::name_of(id),
                    mod01::is_valid(id) ? "oui" : "NON");
    }
    std::printf("\n  Lecon : `enum class` empeche les conversions ACCIDENTELLES,\n");
    std::printf("  mais ne garantit PAS que la valeur appartienne au domaine.\n");
    std::printf("  Toute donnee venant de l'exterieur doit etre validee.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 01 : types, valeurs, memoire                       #\n");
    std::printf("#############################################################\n");

    promotion_entiere();
    comparaison_signee_non_signee();
    debordement();
    initialisation();
    disposition_memoire();
    enumerations();

    std::printf("\nModule 01 termine.\n");
    return 0;
}
