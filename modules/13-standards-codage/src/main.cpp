// =============================================================================
//  Module 13 -- demonstration : standards de codage et analyse statique.
// =============================================================================
#include "mod13/bcd.hpp"

#include <avio/types.hpp>

#include <cstdio>

using avio::u16;
using avio::u32;
using avio::usize;

namespace {

void titre(const char* texte) {
    std::printf("\n=== %s ===\n", texte);
}

// -----------------------------------------------------------------------------
void pourquoi_un_standard() {
    titre("Pourquoi un standard de codage (objectif A-5.4)");
    std::printf("  La DO-178C n'impose AUCUN standard particulier. Elle impose :\n");
    std::printf("    * d'en AVOIR un, decrit dans le SDP (plan de developpement) ;\n");
    std::printf("    * que le code y soit CONFORME (objectif A-5.4) ;\n");
    std::printf("    * que la conformite soit VERIFIEE (revue et/ou analyse) ;\n");
    std::printf("    * que toute DEVIATION soit justifiee et approuvee.\n\n");
    std::printf("  Un standard de codage ne parle pas de correction fonctionnelle.\n");
    std::printf("  Il parle de VERIFIABILITE, de LISIBILITE et de PREVISIBILITE --\n");
    std::printf("  donc du COUT de la verification, de la maintenance et de la\n");
    std::printf("  certification sur vingt ans.\n\n");
    std::printf("  Les trois references du domaine :\n");
    std::printf("    MISRA C++:2023   -- le standard actuel, cible C++17.\n");
    std::printf("                        Fusionne l'heritage d'AUTOSAR C++14.\n");
    std::printf("    AUTOSAR C++14    -- automobile, cible C++14. Historique.\n");
    std::printf("    JSF++ (2005)     -- avionique militaire (F-35). Public et\n");
    std::printf("                        gratuit : une bonne premiere lecture.\n");
}

// -----------------------------------------------------------------------------
void deux_implementations() {
    titre("Le meme algorithme, deux fois : conforme et non conforme");

    const u16 mots[6] = {0x0000U, 0x0001U, 0x1234U, 0x9999U, 0x000AU, 0x12F4U};

    std::printf("  %-10s | %-22s | %-22s\n", "mot BCD", "version CONFORME",
                "version NON CONFORME");
    std::printf("  -----------+------------------------+------------------------\n");

    for (usize index = 0U; index < 6U; ++index) {
        const mod07::Result<u32> conforme = mod13::bcd_to_binary(mots[index]);
        const u32 non_conforme = mod13_nonconforming::bcd_to_binary(mots[index]);

        char texte_conforme[32];
        if (conforme.is_ok()) {
            (void)std::snprintf(texte_conforme, sizeof(texte_conforme), "%u", conforme.value());
        } else {
            (void)std::snprintf(texte_conforme, sizeof(texte_conforme), "ERREUR (%s)",
                                mod07::status_name(conforme.status()));
        }

        std::printf("  0x%04X     | %-22s | %-22u\n", mots[index], texte_conforme, non_conforme);
    }

    std::printf("\n  Resultats FONCTIONNELLEMENT identiques sur le domaine valide.\n");
    std::printf("  Les memes cas de test passent sur les deux versions.\n");
    std::printf("  Aucun test fonctionnel ne fera jamais la difference.\n");
    std::printf("  Seules la REVUE DE CODE et l'ANALYSE STATIQUE la font.\n");
}

// -----------------------------------------------------------------------------
void les_douze_violations() {
    titre("Les douze violations de src/nonconforming.cpp");
    std::printf("  V01  using namespace en portee de fichier\n");
    std::printf("       -> ambiguites de resolution de surcharge, provenance des\n");
    std::printf("          identifiants illisible\n\n");
    std::printf("  V02  macro de type fonction sans parentheses protectrices\n");
    std::printf("       -> DIGIT(x, 1+1) se developpe en (x >> 1+1*4) : FAUX.\n");
    std::printf("          Pas de type, pas de portee, invisible au debogueur\n\n");
    std::printf("  V03  variable globale mutable\n");
    std::printf("       -> couplage de donnees invisible (module 12), fonction non\n");
    std::printf("          reentrante, etat dependant de l'historique des appels\n\n");
    std::printf("  V04  recursion\n");
    std::printf("       -> profondeur de pile non triviale a borner (module 08)\n\n");
    std::printf("  V05  variables non initialisees a la declaration\n");
    std::printf("       -> lecture d'une valeur indeterminee = comportement indefini\n\n");
    std::printf("  V06  nombre magique (4)\n");
    std::printf("  V08  nombre magique (9)\n");
    std::printf("       -> intention non exprimee, modification risquee\n\n");
    std::printf("  V07  conversion de type a la maniere du C\n");
    std::printf("       -> peut supprimer un const en silence, non recherchable\n\n");
    std::printf("  V09  points de sortie multiples + valeur d'erreur sentinelle\n");
    std::printf("       -> rien dans le TYPE ne distingue l'erreur du succes\n\n");
    std::printf("  V10  effet de bord sur une variable globale\n");
    std::printf("  V11  operateur virgule\n");
    std::printf("       -> deux effets de bord dans une expression\n\n");
    std::printf("  V12  fonction jamais appelee : CODE MORT\n");
    std::printf("       -> constat de non-conformite DO-178C 6.4.4.3\n");
}

// -----------------------------------------------------------------------------
void outils() {
    titre("Ce que trouve chaque niveau de verification");
    std::printf("  NIVEAU 1 -- le COMPILATEUR (/W4 /permissive-)\n");
    std::printf("    Trouve : conversions implicites, variables non utilisees,\n");
    std::printf("             membres non initialises, comparaisons signe/non signe.\n");
    std::printf("    Cout : nul. A activer AVANT toute autre chose.\n\n");
    std::printf("  NIVEAU 2 -- l'ANALYSE STATIQUE (clang-tidy)\n");
    std::printf("    Trouve : usage apres deplacement, decoupage, macros\n");
    std::printf("             dangereuses, boucles a compteur flottant, regle de 5\n");
    std::printf("             incomplete, branches identiques.\n");
    std::printf("    Cout : quelques minutes de build. Voir .clang-tidy a la racine.\n\n");
    std::printf("  NIVEAU 3 -- le FORMATAGE (clang-format)\n");
    std::printf("    Ne trouve rien, mais SUPPRIME un sujet de debat en revue et\n");
    std::printf("    rend les differences Git lisibles. Voir .clang-format.\n\n");
    std::printf("  NIVEAU 4 -- la REVUE DE CODE HUMAINE\n");
    std::printf("    Seule capable de juger : le code fait-il ce que dit l'exigence ?\n");
    std::printf("    l'invariant est-il preserve ? le nom est-il juste ?\n");
    std::printf("    Les niveaux 1 a 3 existent pour que la revue se concentre\n");
    std::printf("    la-dessus, et pas sur des points-virgules.\n\n");
    std::printf("  Commandes :\n");
    std::printf("    .\\scripts\\build.ps1 -Preset strict      (compilateur + clang-tidy)\n");
    std::printf("    clang-format -i <fichier>                (formatage)\n");
}

// -----------------------------------------------------------------------------
void deviations() {
    titre("Le processus de deviation");
    std::printf("  Aucun standard n'est applicable a 100 %% sans exception. Ce qui\n");
    std::printf("  compte, ce n'est pas l'absence de deviation : c'est leur\n");
    std::printf("  MAITRISE. Une deviation doit etre :\n\n");
    std::printf("    1. LOCALISEE   -- limitee a la ligne, la fonction ou la cible\n");
    std::printf("                      concernee ; jamais desactivee globalement\n");
    std::printf("    2. JUSTIFIEE   -- pourquoi la regle ne s'applique pas ICI\n");
    std::printf("    3. ANALYSEE    -- quel risque, quelle mesure compensatoire\n");
    std::printf("    4. APPROUVEE   -- par le responsable qualite logicielle\n");
    std::printf("    5. TRACEE      -- enregistree, comptee, revue periodiquement\n\n");
    std::printf("  Exemples reels dans ce depot (cherchez \"DEVIATION\") :\n");
    std::printf("    common/CMakeLists.txt      _CRT_SECURE_NO_WARNINGS, limite a\n");
    std::printf("                               une cible, justifie par la portabilite\n");
    std::printf("    modules/03/.../main.cpp    NOLINT(bugprone-use-after-move) :\n");
    std::printf("                               l'etat post-deplacement EST le sujet\n");
    std::printf("                               du test\n");
    std::printf("    modules/05/.../sensors.cpp NOLINT(performance-unnecessary-\n");
    std::printf("                               value-param) : le decoupage est le\n");
    std::printf("                               defaut a demontrer\n\n");
    std::printf("  Notez la forme : NOLINTNEXTLINE, une seule ligne, precede d'un\n");
    std::printf("  commentaire qui explique POURQUOI. Un `// NOLINT` nu, sans\n");
    std::printf("  justification, est un constat de revue.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 13 : standards de codage et analyse statique       #\n");
    std::printf("#############################################################\n");

    pourquoi_un_standard();
    deux_implementations();
    les_douze_violations();
    outils();
    deviations();

    std::printf("\nModule 13 termine.\n");
    return 0;
}
