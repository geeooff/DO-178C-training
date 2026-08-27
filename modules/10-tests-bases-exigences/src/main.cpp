// =============================================================================
//  Module 10 -- demonstration : conception des tests.
// =============================================================================
#include <avio/types.hpp>
#include <cstdio>
#include <limits>

#include "mod10/alert_monitor.hpp"

using avio::f32;
using avio::u16;
using avio::usize;
using mod10::AlertConfig;
using mod10::AlertMonitor;
using mod10::AlertState;

namespace {

void title(const char* text) {
    std::printf("\n=== %s ===\n", text);
}

AlertMonitor reference_monitor() noexcept {
    AlertConfig config;
    config.raise_threshold = 100.0F;
    config.clear_threshold = 90.0F;
    config.confirm_cycles = 3U;
    config.clear_cycles = 2U;

    AlertMonitor monitor;
    (void)AlertMonitor::create(config, monitor);
    return monitor;
}

void run(const char* label, const f32* profile, usize count) {
    AlertMonitor monitor = reference_monitor();
    std::printf("\n  %s\n", label);
    std::printf("    cycle : ");
    for (usize index = 0U; index < count; ++index) {
        std::printf("%7.1f", static_cast<double>(profile[index]));
    }
    std::printf("\n    etat  : ");
    for (usize index = 0U; index < count; ++index) {
        const AlertState state = monitor.update(profile[index]);
        std::printf("%7.7s", mod10::state_name(state));
    }
    std::printf("\n    -> activations : %u, rejets : %u\n", monitor.activation_count(),
                monitor.rejected_samples());
}

// -----------------------------------------------------------------------------
void state_machine() {
    title("ALERT-MON : seuil 100, retombee 90, confirmation 3, retombee 2");

    const f32 ramp[10] = {80.0F, 95.0F, 105.0F, 110.0F, 115.0F, 95.0F, 85.0F, 80.0F, 85.0F, 95.0F};
    run("Rampe de montee puis de descente", ramp, 10U);

    const f32 noise[10] = {150.0F, 95.0F,  150.0F, 95.0F,  150.0F,
                           95.0F,  150.0F, 95.0F,  150.0F, 95.0F};
    run("Bruit de capteur (un cycle sur deux au-dessus du seuil)", noise, 10U);

    const f32 oscillation[10] = {150.0F, 150.0F, 150.0F, 85.0F, 95.0F,
                                 85.0F,  95.0F,  85.0F,  95.0F, 85.0F};
    run("Oscillation autour du seuil de retombee", oscillation, 10U);

    const f32 fault[8] = {150.0F,
                          150.0F,
                          std::numeric_limits<f32>::quiet_NaN(),
                          std::numeric_limits<f32>::infinity(),
                          150.0F,
                          std::numeric_limits<f32>::quiet_NaN(),
                          85.0F,
                          85.0F};
    run("Capteur intermittent (NaN / infini)", fault, 8U);
}

// -----------------------------------------------------------------------------
void design_techniques() {
    title("Les techniques de conception des cas de test");

    std::printf("  1. CLASSES D'EQUIVALENCE\n");
    std::printf("     Partitionner le domaine d'entree en classes produisant un\n");
    std::printf("     comportement qualitativement identique, puis prendre UN\n");
    std::printf("     representant par classe.\n");
    std::printf("       C1 : v > 100        contribue a la montee\n");
    std::printf("       C2 : 90 <= v <= 100 zone morte (hysteresis)\n");
    std::printf("       C3 : v < 90         contribue a la retombee\n");
    std::printf("     Tester 150 PUIS 200 n'apporte rien : meme classe.\n\n");

    std::printf("  2. ANALYSE DES VALEURS LIMITES\n");
    std::printf("     Les defauts se concentrent aux frontieres. Pour chaque seuil :\n");
    std::printf("     la valeur EXACTE, juste en dessous, juste au-dessus.\n");
    std::printf("       update(100,000) -> Inactive   (la condition est \"> 100\")\n");
    std::printf("       update(100,001) -> Pending\n");
    std::printf("     Un `>` ecrit `>=` par erreur ne se voit QUE la.\n\n");

    std::printf("  3. COUVERTURE DES ETATS ET DES TRANSITIONS\n");
    std::printf("     4 etats, 6 transitions nommees + les auto-transitions.\n");
    std::printf("     Atteindre les 4 etats ne suffit PAS : les defauts se cachent\n");
    std::printf("     dans les transitions d'ANNULATION (Pending -> Inactive et\n");
    std::printf("     Clearing -> Active), les plus souvent oubliees.\n\n");

    std::printf("  4. SEQUENCES\n");
    std::printf("     Enchainements realistes : bruit, oscillation, rampe, panne\n");
    std::printf("     intermittente. Ils detectent ce que les tests unitaires par\n");
    std::printf("     transition laissent passer (compteur non reinitialise...).\n\n");

    std::printf("  5. ROBUSTESSE (DO-178C 6.4.2.2)\n");
    std::printf("     Entrees hors domaine, non finies, parametres degeneres.\n");
    std::printf("     C'est la moitie de la campagne, et celle qui trouve les\n");
    std::printf("     vrais defauts.\n");
}

// -----------------------------------------------------------------------------
void what_the_test_tests() {
    title("Revue de test : est-ce que le test teste vraiment ?");
    std::printf("  Un test qui passe ne prouve rien s'il passerait AUSSI avec un\n");
    std::printf("  code faux. La technique de verification s'appelle l'ANALYSE DE\n");
    std::printf("  MUTATION : on introduit volontairement un defaut, et on verifie\n");
    std::printf("  qu'AU MOINS UN test echoue.\n\n");
    std::printf("  Mutations a essayer sur alert_monitor.cpp :\n");
    std::printf("    M1 : remplacer `sample > raise` par `sample >= raise`\n");
    std::printf("    M2 : supprimer `progress_ = 0U` dans l'annulation Pending\n");
    std::printf("    M3 : incrementer activations_ aussi depuis Clearing\n");
    std::printf("    M4 : remplacer `>= confirm_cycles` par `> confirm_cycles`\n");
    std::printf("    M5 : traiter un NaN comme une valeur sous le seuil\n\n");
    std::printf("  Pour chacune : quel test echoue ? Si aucun n'echoue, la\n");
    std::printf("  campagne a un trou, et il faut ajouter un cas de test.\n");
    std::printf("  C'est l'exercice 5.1 du README.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 10 : tests bases sur les exigences                 #\n");
    std::printf("#############################################################\n");

    state_machine();
    design_techniques();
    what_the_test_tests();

    std::printf("\nModule 10 termine.\n");
    return 0;
}
