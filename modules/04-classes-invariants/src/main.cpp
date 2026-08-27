// =============================================================================
//  Module 04 -- demonstration : classes, invariants, types forts.
// =============================================================================
#include <avio/types.hpp>
#include <cstdio>
#include <limits>

#include "mod04/units.hpp"

using avio::f32;
using avio::i32;
using mod04::Altitude;
using mod04::FuelTank;
using mod04::Mass;

namespace {

void title(const char* text) {
    std::printf("\n=== %s ===\n", text);
}

Mass grams(i32 value) noexcept {
    Mass mass;
    (void)Mass::from_grams(value, mass);
    return mass;
}

// -----------------------------------------------------------------------------
void unit_confusion() {
    title("Le vol Air Canada 143, ou 22 300 unites de carburant");

    Mass pounds;
    Mass kilograms;
    (void)Mass::from_pounds(22300.0F, pounds);
    (void)Mass::from_kilograms(22300.0F, kilograms);

    std::printf("  22300 interprete en LIVRES      : %10.1f kg\n",
                static_cast<double>(pounds.kilograms()));
    std::printf("  22300 interprete en KILOGRAMMES : %10.1f kg\n",
                static_cast<double>(kilograms.kilograms()));
    std::printf("  ecart                            : %10.1f kg\n",
                static_cast<double>((kilograms - pounds).kilograms()));

    std::printf("\n  En 1983, cet ecart a vide les reservoirs d'un Boeing 767 a\n");
    std::printf("  12 500 m d'altitude. L'equipage a pose l'appareil en vol plane.\n");
    std::printf("\n  Avec un type fort, la question ne se pose plus :\n");
    std::printf("    Mass m; Mass::from_pounds(22300.0F, m);     <- l'unite est DANS l'appel\n");
    std::printf("    void charger(Mass m);                        <- plus aucune ambiguite\n");
    std::printf("  Comparez avec :  void charger(float quantite); <- quelle unite ?\n");
}

// -----------------------------------------------------------------------------
void what_does_not_compile() {
    title("Ce que le type fort REFUSE de compiler");
    std::printf("  Altitude a; Mass m;\n\n");
    std::printf("    a = m;                  // erreur : types incompatibles\n");
    std::printf("    if (a < m) { }          // erreur : pas d'operateur\n");
    std::printf("    Altitude b = 35000.0F;  // erreur : constructeur prive\n");
    std::printf("    f32 x = a;              // erreur : pas de conversion implicite\n\n");
    std::printf("  Chacune de ces erreurs serait, avec des `float` nus, un bug\n");
    std::printf("  silencieux decouvert en integration -- ou en vol.\n");
    std::printf("\n  Cout a l'execution : ZERO. sizeof(Altitude) = %zu, sizeof(f32) = %zu.\n",
                sizeof(Altitude), sizeof(f32));
    std::printf("  Le compilateur genere exactement le meme code machine.\n");
}

// -----------------------------------------------------------------------------
void boundary_validation() {
    title("Validation aux frontieres : NaN et infini s'arretent ici");

    const f32 inputs[5] = {35000.0F, -5000.0F, 80000.0F, std::numeric_limits<f32>::quiet_NaN(),
                           std::numeric_limits<f32>::infinity()};
    const char* labels[5] = {"35000 ft (nominal)", "-5000 ft (sous le domaine)",
                             "80000 ft (au-dessus)", "NaN", "+infini"};

    for (avio::usize index = 0U; index < 5U; ++index) {
        Altitude altitude;
        const bool accepted = Altitude::from_feet(inputs[index], altitude);
        std::printf("  %-28s -> %s\n", labels[index], accepted ? "ACCEPTE" : "REJETE");
    }

    std::printf("\n  Un NaN qui entre dans un calcul en ressort partout : toute\n");
    std::printf("  comparaison avec un NaN est fausse, y compris `x == x`. La seule\n");
    std::printf("  strategie tenable est de le bloquer A L'ENTREE du systeme.\n");
}

// -----------------------------------------------------------------------------
void tank_invariant() {
    title("Un invariant qui tient, quelles que soient les demandes");

    FuelTank tank;
    (void)FuelTank::create(grams(5000), tank);

    const i32 requests[6] = {2000, 4000, -1000, -9000, 500, -500};
    std::printf("  capacite : %d g\n\n", tank.capacity().grams());
    std::printf("  %-22s %10s %10s %8s\n", "operation", "demande", "effectif", "contenu");
    std::printf("  ---------------------------------------------------------\n");

    for (avio::usize index = 0U; index < 6U; ++index) {
        const i32 request = requests[index];
        i32 effective = 0;
        if (request >= 0) {
            effective = tank.add(grams(request)).grams();
        } else {
            effective = -tank.remove(grams(-request)).grams();
        }
        std::printf("  %-22s %10d %10d %8d   invariant : %s\n",
                    (request >= 0) ? "ajout" : "prelevement", request, effective,
                    tank.quantity().grams(), tank.invariant_holds() ? "OK" : "VIOLE");
    }

    std::printf("\n  Remplissage : %.1f %%\n", static_cast<double>(tank.fill_ratio_percent()));
    std::printf("\n  L'appelant ne peut PAS violer l'invariant, meme en demandant\n");
    std::printf("  n'importe quoi. C'est la difference entre une classe et une\n");
    std::printf("  structure de donnees accompagnee d'un mode d'emploi.\n");
}

// -----------------------------------------------------------------------------
void float_equality() {
    title("Pourquoi Altitude n'a PAS d'operator==");

    const double a = 0.1 + 0.2;
    std::printf("  0.1 + 0.2 == 0.3 ?  ->  %s   (0.1+0.2 = %.17g)\n", (a == 0.3) ? "true" : "false",
                a);

    Altitude x;
    Altitude y;
    (void)Altitude::from_feet(10000.0F, x);
    (void)Altitude::from_meters(3048.0F, y);  // 10000 ft = 3048 m exactement

    std::printf("\n  10000 ft construit depuis des pieds : %.6f ft\n",
                static_cast<double>(x.feet()));
    std::printf("  10000 ft construit depuis des metres : %.6f ft\n",
                static_cast<double>(y.feet()));
    std::printf("  is_close(tolerance = 0,01 ft)        : %s\n",
                x.is_close(y, 0.01F) ? "oui" : "non");
    std::printf("  is_close(tolerance = 0 ft)           : %s\n",
                x.is_close(y, 0.0F) ? "oui" : "non");

    std::printf("\n  Ne pas fournir operator== est un CHOIX DE CONCEPTION : il force\n");
    std::printf("  l'appelant a expliciter sa tolerance, donc a y reflechir. En\n");
    std::printf("  DO-178C, cette tolerance appartient a l'exigence, pas au code.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 04 : classes, invariants, types forts              #\n");
    std::printf("#############################################################\n");

    unit_confusion();
    what_does_not_compile();
    boundary_validation();
    tank_invariant();
    float_equality();

    std::printf("\nModule 04 termine.\n");
    return 0;
}
