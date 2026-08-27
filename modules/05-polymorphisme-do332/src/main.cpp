// =============================================================================
//  Module 05 -- demonstration : polymorphisme et DO-332.
// =============================================================================
#include <avio/types.hpp>
#include <cstdio>

#include "mod05/sensors.hpp"

using avio::f32;
using avio::i32;
using avio::u16;
using avio::usize;

namespace {

void title(const char* text) {
    std::printf("\n=== %s ===\n", text);
}

void print_report(const mod05::Sensor& sensor) {
    const mod05::ContractReport report = mod05::verify_contract(sensor, 200U);
    const char* const yes_no[2] = {"NON", "oui"};

    std::printf(
        "  %-16s C1:%-3s C2:%-3s C3:%-3s C4:%-3s C5:%-3s C6:%-3s C7:%-3s  -> %s\n", sensor.name(),
        yes_no[report.c1_name_valid ? 1 : 0], yes_no[report.c2_raw_domain_valid ? 1 : 0],
        yes_no[report.c3_value_domain_valid ? 1 : 0], yes_no[report.c4_result_in_domain ? 1 : 0],
        yes_no[report.c5_monotonic ? 1 : 0], yes_no[report.c6_endpoints_match ? 1 : 0],
        yes_no[report.c7_clamped_outside ? 1 : 0],
        report.all_satisfied() ? "CONFORME" : "NON CONFORME");
    if (!report.all_satisfied()) {
        std::printf("                   premiere clause violee : %s\n", report.first_violation());
    }
}

// -----------------------------------------------------------------------------
void resolution_dynamique() {
    title("Resolution dynamique : un seul code, plusieurs comportements");

    const mod05::PressureSensor pressure;
    const mod05::TemperatureSensor temperature;
    const mod05::Sensor* sensors[2] = {&pressure, &temperature};

    const i32 raw_values[4] = {0, 1365, 2730, 4095};

    std::printf("  %-14s %10s %10s %10s %10s\n", "capteur", "brut 0", "brut 1365", "brut 2730",
                "brut 4095");
    std::printf("  ----------------------------------------------------------------\n");
    for (usize index = 0U; index < 2U; ++index) {
        std::printf("  %-14s", sensors[index]->name());
        for (usize k = 0U; k < 4U; ++k) {
            std::printf(" %10.2f",
                        static_cast<double>(sensors[index]->to_engineering(raw_values[k])));
        }
        std::printf("\n");
    }

    std::printf("\n  L'appel `capteurs[i]->to_engineering(x)` est identique dans les\n");
    std::printf("  deux cas : c'est la TABLE DES FONCTIONS VIRTUELLES de l'objet qui\n");
    std::printf("  decide. Cout : une indirection memoire par appel, et un pointeur\n");
    std::printf("  cache de %zu octets dans chaque objet.\n", sizeof(void*));
}

// -----------------------------------------------------------------------------
void local_type_consistency() {
    title("DO-332 : coherence locale de type (objectif OO.6.7)");

    std::printf("  Le contrat de Sensor (7 clauses) est rejoue sur CHAQUE sous-type.\n\n");

    const mod05::PressureSensor pressure;
    const mod05::TemperatureSensor temperature;
    const mod05::BrokenSensor faulty;

    print_report(pressure);
    print_report(temperature);
    print_report(faulty);

    std::printf("\n  Le capteur fautif COMPILE, s'utilise normalement et passerait\n");
    std::printf("  n'importe quel test nominal ecrit a la va-vite. Seul le rejeu du\n");
    std::printf("  contrat de la classe de base revele qu'il n'est pas substituable.\n");
    std::printf("\n  C'est exactement ce que demande la DO-332 : demontrer, pour chaque\n");
    std::printf("  sous-type, qu'il respecte le contrat du type de base.\n");
}

// -----------------------------------------------------------------------------
void split() {
    title("Decoupage (slicing) : le sous-type qui disparait");

    const mod05::ExtendedMessage message(0x101U, 20U);

    std::printf("  ExtendedMessage(id=0x101, charge utile=20) -> length() = %u\n",
                message.length());
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    const u16 by_value = mod05::length_by_value(message);
    const u16 by_reference = mod05::length_by_reference(message);

    std::printf("  passe PAR VALEUR      : length() = %u   <-- DECOUPE\n", by_value);
    std::printf("  passe PAR REFERENCE   : length() = %u   <-- correct\n", by_reference);

    std::printf("\n  Le passage par valeur copie UNIQUEMENT la partie `Message` :\n");
    std::printf("  la charge utile est perdue, et la resolution dynamique avec elle.\n");
    std::printf("  Aucun avertissement du compilateur par defaut. clang-tidy le\n");
    std::printf("  detecte (cppcoreguidelines-slicing), et MISRA l'interdit.\n");
    std::printf("\n  PARADE : interdire la copie sur toute classe de base polymorphe\n");
    std::printf("  (`Sensor(const Sensor&) = delete;`). Le probleme devient alors une\n");
    std::printf("  erreur de compilation.\n");
}

// -----------------------------------------------------------------------------
void vulnerabilities_do332() {
    title("Les six vulnerabilites identifiees par la DO-332");
    std::printf("  1. Heritage / polymorphisme  -> coherence locale de type   (ce module)\n");
    std::printf("  2. Polymorphisme parametrique -> couverture par instanciation (module 06)\n");
    std::printf("  3. Surcharge de fonctions     -> ambiguite de resolution     (module 06)\n");
    std::printf("  4. Conversions de type        -> downcast, RTTI, slicing     (ce module)\n");
    std::printf("  5. Memoire dynamique          -> fragmentation, epuisement   (module 08)\n");
    std::printf("  6. Exceptions                 -> flot de controle implicite  (module 07)\n");
    std::printf("\n  Regles pratiques appliquees dans ce depot :\n");
    std::printf("    * hierarchies PLATES : une interface abstraite, des feuilles `final`\n");
    std::printf("    * pas d'heritage multiple d'implementation\n");
    std::printf("    * pas de dynamic_cast, pas de typeid : RTTI desactivee en production\n");
    std::printf("    * destructeur virtuel obligatoire (verifie par static_assert)\n");
    std::printf("    * classes de base polymorphes non copiables\n");
    std::printf("    * `override` systematique, `final` des que possible\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 05 : polymorphisme et supplement DO-332            #\n");
    std::printf("#############################################################\n");

    resolution_dynamique();
    local_type_consistency();
    split();
    vulnerabilities_do332();

    std::printf("\nModule 05 termine.\n");
    return 0;
}
