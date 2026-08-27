// =============================================================================
//  Module 15 -- demonstration : determinisme, flottants, temps reel.
// =============================================================================
#include <avio/types.hpp>
#include <cstdio>

#include "mod15/fixed_point.hpp"
#include "mod15/float_facts.hpp"
#include "mod15/hw_register.hpp"
#include "mod15/schedule.hpp"

using avio::f32;
using avio::u32;
using avio::usize;
using mod15::Fixed;
using mod15::MajorFrame;
using mod15::PartitionId;

namespace {

void titre(const char* texte) {
    std::printf("\n=== %s ===\n", texte);
}

// -----------------------------------------------------------------------------
void faits_ieee754() {
    titre("Trois faits sur IEEE-754 que tout embarqueur doit connaitre");

    std::printf("  1. LA PRECISION DEPEND DE LA MAGNITUDE\n");
    const f32 magnitudes[5] = {1.0F, 1024.0F, 65536.0F, 8388608.0F, 16777216.0F};
    std::printf("     %14s %16s\n", "valeur", "ULP");
    for (usize index = 0U; index < 5U; ++index) {
        std::printf("     %14.1f %16.9g\n", static_cast<double>(magnitudes[index]),
                    static_cast<double>(mod15::ulp(magnitudes[index])));
    }
    std::printf("\n     A 2^24 = 16 777 216, l'ULP vaut 2,0 :\n");
    std::printf("     16777216.0F + 1.0F == 16777216.0F  ->  %s\n",
                mod15::is_absorbed(16777216.0F, 1.0F) ? "VRAI (absorption)" : "faux");
    std::printf("     C'est aussi pourquoi 1.0F + 1e-9F ne change rien : %s\n",
                mod15::is_absorbed(1.0F, 1.0e-9F) ? "absorbe" : "non absorbe");

    std::printf("\n  2. L'ADDITION N'EST PAS ASSOCIATIVE\n");
    std::printf("     (1 + (-1)) + 1e-8  vaut 1e-8\n");
    std::printf("      1 + ((-1) + 1e-8) vaut 0     car -1 + 1e-8 s'arrondit a -1\n");
    std::printf("     -> different : %s\n",
                mod15::addition_is_non_associative(1.0F, -1.0F, 1.0e-8F) ? "OUI" : "non");
    std::printf("     Consequence : le compilateur n'a PAS le droit de reordonner\n");
    std::printf("     une somme flottante... sauf avec /fp:fast, ou il se l'autorise.\n");
    std::printf("     Deux jeux d'options, deux resultats. D'ou /fp:precise.\n");

    std::printf("\n  3. L'ERREUR S'ACCUMULE\n");
    std::printf("     %-32s %20s %14s\n", "somme de N fois 0,1F", "resultat", "erreur");
    const u32 comptes[3] = {10U, 100U, 1000U};
    const double attendus[3] = {1.0, 10.0, 100.0};
    for (usize index = 0U; index < 3U; ++index) {
        const f32 total = mod15::accumulate_float(0.1F, comptes[index]);
        std::printf("     N = %-28u %20.9f %14.3g\n", comptes[index], static_cast<double>(total),
                    static_cast<double>(total) - attendus[index]);
    }
    const f32 kahan = mod15::accumulate_kahan(0.1F, 1000U);
    std::printf("     N = 1000, somme de KAHAN     %20.9f %14.3g\n", static_cast<double>(kahan),
                static_cast<double>(kahan) - 100.0);
    std::printf("\n     La somme de Kahan recupere l'erreur d'arrondi a chaque etape.\n");
    std::printf("     Attention : elle EXIGE /fp:precise -- avec /fp:fast, le\n");
    std::printf("     compilateur la simplifie algebriquement et l'annule.\n");
}

// -----------------------------------------------------------------------------
void virgule_fixe() {
    titre("Virgule fixe Q16.16 : la precision previsible");

    std::printf("  Format : 16 bits entiers signes + 16 bits fractionnaires\n");
    std::printf("    domaine    : [-32768 ; +32767]\n");
    std::printf("    resolution : 1/65536 = %.9g  (CONSTANTE sur tout le domaine)\n",
                static_cast<double>(Fixed::kResolution));

    std::printf("\n  Representation de quelques valeurs :\n");
    const f32 valeurs[5] = {1.0F, 0.5F, 0.25F, 0.1F, -1.5F};
    std::printf("    %10s %14s %18s\n", "valeur", "brut (i32)", "relu");
    for (usize index = 0U; index < 5U; ++index) {
        const Fixed fixe = Fixed::from_float(valeurs[index]);
        std::printf("    %10.4f %14d %18.9f\n", static_cast<double>(valeurs[index]), fixe.raw(),
                    static_cast<double>(fixe.to_float()));
    }
    std::printf("\n    0,5 et 0,25 sont EXACTS (puissances de deux).\n");
    std::printf("    0,1 ne l'est pas : 0,1 x 65536 = 6553,6, arrondi a 6554.\n");
    std::printf("    L'erreur vaut donc 6554/65536 - 0,1 = 6,1e-6. Ce chiffre se\n");
    std::printf("    calcule A LA MAIN, avant toute execution.\n");

    std::printf("\n  Accumulation de 1000 fois 0,1 :\n");
    const Fixed total_fixe = mod15::accumulate(Fixed::from_float(0.1F), 1000U);
    const f32 total_flottant = mod15::accumulate_float(0.1F, 1000U);
    std::printf("    virgule fixe : brut = %d = 1000 x 6554  ->  %.9f\n", total_fixe.raw(),
                static_cast<double>(total_fixe.to_float()));
    std::printf("    flottant     :                            %.9f\n",
                static_cast<double>(total_flottant));

    std::printf("\n  Notez bien : la virgule fixe n'est pas forcement PLUS PRECISE.\n");
    std::printf("  Ici son erreur est meme plus grande. Ce qu'elle apporte, c'est\n");
    std::printf("  que le resultat est EXACTEMENT 6554000, calculable a la main,\n");
    std::printf("  identique sur toute cible, et verifiable par une comparaison\n");
    std::printf("  d'ENTIERS. C'est la PREVISIBILITE que l'on achete, pas la\n");
    std::printf("  precision.\n");

    std::printf("\n  Et l'egalite exacte redevient legitime :\n");
    std::printf("    Fixed(0,5) + Fixed(0,5) == Fixed(1,0)  ->  %s\n",
                ((Fixed::from_float(0.5F) + Fixed::from_float(0.5F)) == Fixed::from_int(1))
                    ? "VRAI"
                    : "faux");
    std::printf("    0.5F + 0.5F == 1.0F                    ->  vrai (ici, par chance)\n");
    std::printf("    0.1F + 0.2F == 0.3F                    ->  %s\n",
                ((0.1F + 0.2F) == 0.3F) ? "vrai" : "FAUX");
}

// -----------------------------------------------------------------------------
void ordonnancement() {
    titre("Ordonnancement a fenetres fixes (ARINC 653)");

    MajorFrame frame(10000U);
    (void)frame.add_window(PartitionId::FlightControl, 0U, 3000U);
    (void)frame.add_window(PartitionId::FuelManagement, 3000U, 1500U);
    (void)frame.add_window(PartitionId::Display, 4500U, 1500U);
    (void)frame.add_window(PartitionId::Maintenance, 6000U, 1000U);

    std::printf("  Trame majeure de %u us (%u Hz) :\n\n", frame.period_us(),
                1000000U / frame.period_us());
    std::printf("    %-26s %10s %10s %10s\n", "partition", "debut", "duree", "fin");
    std::printf("    --------------------------------------------------------------\n");
    for (usize index = 0U; index < frame.window_count(); ++index) {
        const mod15::Window& fenetre = frame.window(index);
        std::printf("    %-26s %9u %9u %9u\n", mod15::partition_name(fenetre.partition),
                    fenetre.offset_us, fenetre.duration_us,
                    fenetre.offset_us + fenetre.duration_us);
    }
    std::printf("\n    alloue      : %u us (%u %%)\n", frame.allocated_us(),
                frame.utilisation_percent());
    std::printf("    marge       : %u us (%u %%)\n", frame.slack_us(),
                (frame.slack_us() * 100U) / frame.period_us());
    std::printf("    marge >= 20 %% : %s\n", frame.has_margin(20U) ? "OUI" : "NON");

    std::printf("\n  Execution nominale :\n");
    const u32 nominal[4] = {2500U, 1200U, 1400U, 800U};
    mod15::FrameResult resultat = mod15::run_major_frame(frame, nominal, 4U);
    std::printf("    echeances tenues : %s, depassements : %zu\n",
                resultat.deadline_met ? "OUI" : "non", resultat.overrun_count);

    std::printf("\n  La partition Maintenance (DAL D) part en boucle :\n");
    const u32 degrade[4] = {2500U, 1200U, 1400U, 9000U};
    resultat = mod15::run_major_frame(frame, degrade, 4U);
    std::printf("    echeances tenues : %s\n", resultat.deadline_met ? "oui" : "NON");
    std::printf("    depassements     : %zu\n", resultat.overrun_count);
    std::printf("    pire depassement : %u us, partition %s\n", resultat.worst_overrun_us,
                mod15::partition_name(resultat.worst_partition));

    std::printf("\n  POINT DECISIF : la partition fautive est INTERROMPUE a la fin de\n");
    std::printf("  sa fenetre. Elle ne vole pas une microseconde aux commandes de vol.\n");
    std::printf("  C'est cela, la \"freedom from interference\" : une fonction DAL D\n");
    std::printf("  ne peut ni corrompre la memoire, ni retarder une fonction DAL A\n");
    std::printf("  qui partage le meme calculateur.\n");
    std::printf("\n  Sans partitionnement, TOUT le calculateur devrait etre developpe\n");
    std::printf("  au niveau le plus severe. C'est le coeur du modele IMA.\n");
}

// -----------------------------------------------------------------------------
void wcet() {
    titre("Le WCET : ce qu'on doit demontrer");
    std::printf("  WCET = Worst-Case Execution Time. Il faut demontrer que chaque\n");
    std::printf("  partition tient dans sa fenetre, DANS LE PIRE CAS -- pas en moyenne.\n\n");
    std::printf("  DEUX APPROCHES :\n");
    std::printf("    MESURE      : on execute et on mesure. Simple, mais on ne mesure\n");
    std::printf("                  que les chemins que l'on a su declencher. Le pire\n");
    std::printf("                  cas reel peut n'avoir jamais ete atteint.\n");
    std::printf("    ANALYSE STATIQUE : on analyse le binaire et le modele du\n");
    std::printf("                  processeur (caches, pipeline, predicteur). Donne\n");
    std::printf("                  une borne SURE, mais pessimiste. Outils du marche :\n");
    std::printf("                  aiT d'AbsInt, RapiTime de Rapita.\n");
    std::printf("    En pratique : les deux, et on compare.\n\n");
    std::printf("  CE QUI REND LE WCET CALCULABLE (tout le cours y menait) :\n");
    std::printf("    * boucles a bornes connues              (modules 08, 15)\n");
    std::printf("    * pas de recursion                      (module 08)\n");
    std::printf("    * pas d'allocation dynamique            (module 08)\n");
    std::printf("    * pas d'exceptions                      (module 07)\n");
    std::printf("    * pas d'appel virtuel non resolu        (modules 05, 06)\n");
    std::printf("    * pas d'attente active non bornee       (module 15)\n");
    std::printf("    * graphe d'appel statiquement analysable (module 12)\n\n");
    std::printf("  Chacune de ces regles a semble arbitraire quand on l'a rencontree.\n");
    std::printf("  Elles convergent toutes vers le meme objectif : rendre le\n");
    std::printf("  comportement du logiciel PREVISIBLE ET DEMONTRABLE.\n");
}

// -----------------------------------------------------------------------------
void registres_et_volatile() {
    titre("volatile : ce qu'il garantit, ce qu'il ne garantit pas");

    mod15::SimulatedRegister registre;
    registre.set_hardware_value(0x0000U);
    const bool sans_reponse = mod15::wait_for_bit(registre, 0x0004U, 50U);
    std::printf("  Materiel muet   : attente = %s apres %u lectures (budget 50)\n",
                sans_reponse ? "reussie" : "ECHOUEE", registre.read_count());

    mod15::SimulatedRegister pret;
    pret.set_hardware_value(0x0004U);
    const bool reponse = mod15::wait_for_bit(pret, 0x0004U, 50U);
    std::printf("  Materiel pret   : attente = %s apres %u lecture(s)\n",
                reponse ? "reussie" : "echouee", pret.read_count());

    std::printf("\n  GARANTIT : chaque lecture et chaque ecriture produit un acces\n");
    std::printf("  memoire reel. Sans volatile, ce code est une boucle infinie :\n\n");
    std::printf("      while (registre_etat == 0U) { }   // lu UNE fois, puis cache\n\n");
    std::printf("  NE GARANTIT PAS : l'atomicite, l'ordre vis-a-vis des acces non\n");
    std::printf("  volatiles, l'exclusion mutuelle, les barrieres memoire.\n\n");
    std::printf("  Pour la CONCURRENCE, c'est std::atomic qu'il faut, pas volatile.\n");
    std::printf("  La confusion entre les deux est l'une des erreurs les plus\n");
    std::printf("  repandues du developpement embarque :\n");
    std::printf("      volatile     s'adresse au MATERIEL\n");
    std::printf("      std::atomic  s'adresse aux AUTRES FILS D'EXECUTION\n\n");
    std::printf("  ET SURTOUT : l'attente est BORNEE. Une attente active sans borne\n");
    std::printf("  laisse le chien de garde redemarrer le calculateur -- evenement\n");
    std::printf("  bien plus grave, en vol, que la panne du peripherique.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 15 : determinisme, flottants, temps reel           #\n");
    std::printf("#############################################################\n");

    faits_ieee754();
    virgule_fixe();
    ordonnancement();
    wcet();
    registres_et_volatile();

    std::printf("\nModule 15 termine.\n");
    return 0;
}
