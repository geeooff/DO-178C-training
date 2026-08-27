#include <avio/types.hpp>
#include <limits>
#include <microtest/microtest.hpp>

#include "mod15/fixed_point.hpp"
#include "mod15/float_facts.hpp"
#include "mod15/hw_register.hpp"
#include "mod15/schedule.hpp"

using avio::f32;
using avio::i32;
using avio::u32;
using avio::usize;
using mod15::Fixed;
using mod15::MajorFrame;
using mod15::PartitionId;

namespace {

constexpr f32 kNaN = std::numeric_limits<f32>::quiet_NaN();
constexpr f32 kInf = std::numeric_limits<f32>::infinity();

}  // namespace

// =============================================================================
//  1. Faits IEEE-754, demontres et non pas affirmes
// =============================================================================
TEST_REQ(Flottant, absorption_au_dela_de_deux_puissance_24, "LLR-DET-010") {
    // A 2^24, l'ecart entre deux flottants simple precision vaut 2,0.
    // Ajouter 1,0 ne change donc RIEN.
    CHECK(mod15::is_absorbed(16777216.0F, 1.0F));
    // Juste en dessous, l'ecart vaut 1,0 : l'addition passe encore.
    CHECK_FALSE(mod15::is_absorbed(8388608.0F, 1.0F));
    CHECK_FALSE(mod15::is_absorbed(1.0F, 1.0F));
}

TEST_REQ(Flottant, absorption_a_petite_echelle, "LLR-DET-010") {
    // L'absorption n'est pas reservee aux grands nombres : elle depend du
    // RAPPORT entre les deux operandes.
    CHECK(mod15::is_absorbed(1.0F, 1.0e-9F));
    CHECK_FALSE(mod15::is_absorbed(1.0F, 1.0e-6F));
}

TEST_REQ(Flottant, ulp_croit_avec_la_magnitude, "LLR-DET-011") {
    // 2^-23 = 1,1920929e-7
    CHECK_NEAR(static_cast<double>(mod15::ulp(1.0F)), 1.1920929e-7, 1e-13);
    // A 1024, l'ULP est 1024 fois plus grand.
    CHECK_NEAR(static_cast<double>(mod15::ulp(1024.0F)), 1.220703125e-4, 1e-10);
    // A 2^24, l'ULP vaut 2,0 : d'ou l'absorption du test precedent.
    CHECK_NEAR(static_cast<double>(mod15::ulp(16777216.0F)), 2.0, 1e-9);

    // C'EST LA PROPRIETE CLE : la precision d'un flottant DEPEND DE SA
    // MAGNITUDE. Une tolerance absolue valable a 1,0 ne l'est plus a 1e7.
    CHECK(mod15::ulp(1.0F) < mod15::ulp(1024.0F));
    CHECK(mod15::ulp(1024.0F) < mod15::ulp(16777216.0F));
    CHECK_NEAR(static_cast<double>(mod15::ulp(kNaN)), 0.0, 1e-12);
}

TEST_REQ(Flottant, addition_non_associative, "LLR-DET-012") {
    // (1 + (-1)) + 1e-8 = 1e-8
    //  1 + ((-1) + 1e-8) = 0   car -1 + 1e-8 vaut -1 apres arrondi
    CHECK(mod15::addition_is_non_associative(1.0F, -1.0F, 1.0e-8F));
    CHECK(mod15::addition_is_non_associative(1.0e7F, -1.0e7F, 0.5F));

    // Sur des valeurs de magnitude comparable, l'associativite tient.
    CHECK_FALSE(mod15::addition_is_non_associative(1.0F, 2.0F, 3.0F));

    // CONSEQUENCE : le compilateur n'a PAS le droit de reordonner une somme
    // flottante... sauf avec /fp:fast, ou il se l'autorise. Deux jeux
    // d'options, deux resultats. D'ou /fp:precise, impose dans
    // cmake/TrainingHelpers.cmake.
}

TEST_REQ(Flottant, derive_d_accumulation, "LLR-DET-013") {
    // 0,1 n'est pas representable exactement en binaire. Additionner dix fois
    // 0,1F ne donne PAS 1,0F.
    const f32 ten_times = mod15::accumulate_float(0.1F, 10U);
    CHECK(ten_times != 1.0F);
    CHECK_NEAR(static_cast<double>(ten_times), 1.0, 1e-6);

    // Et la derive CROIT avec le nombre de termes.
    const f32 thousand_times = mod15::accumulate_float(0.1F, 1000U);
    CHECK(thousand_times != 100.0F);
    const double error_10 = static_cast<double>(ten_times) - 1.0;
    const double error_1000 = static_cast<double>(thousand_times) - 100.0;
    const double abs_10 = (error_10 < 0.0) ? -error_10 : error_10;
    const double abs_1000 = (error_1000 < 0.0) ? -error_1000 : error_1000;
    CHECK(abs_1000 > abs_10);
}

TEST_REQ(Flottant, somme_de_kahan_borne_la_derive, "LLR-DET-014") {
    // La somme de Kahan recupere l'erreur d'arrondi a chaque etape : la derive
    // ne croit plus avec le nombre de termes.
    const f32 naive = mod15::accumulate_float(0.1F, 1000U);
    const f32 kahan = mod15::accumulate_kahan(0.1F, 1000U);

    const double error_naive = static_cast<double>(naive) - 100.0;
    const double error_kahan = static_cast<double>(kahan) - 100.0;
    const double abs_naive = (error_naive < 0.0) ? -error_naive : error_naive;
    const double abs_kahan = (error_kahan < 0.0) ? -error_kahan : error_kahan;

    CHECK(abs_kahan <= abs_naive);
    CHECK(abs_kahan < 1.0e-4);
    CHECK(abs_naive > 1.0e-5);
}

TEST_REQ(Flottant, tolerance_absolue_et_relative, "LLR-DET-020,LLR-DET-021") {
    // Tolerance ABSOLUE : convient aux grandeurs a domaine borne.
    CHECK(mod15::close_absolute(1000.0F, 1000.5F, 1.0F));
    CHECK_FALSE(mod15::close_absolute(1000.0F, 1002.0F, 1.0F));

    // Le piege : la MEME tolerance absolue devient absurde a grande echelle.
    // 1 ft d'ecart sur 1 000 000, c'est du bruit ; sur 10, c'est 10 %.
    CHECK(mod15::close_absolute(1000000.0F, 1000000.5F, 1.0F));
    CHECK_FALSE(mod15::close_absolute(10.0F, 11.0F, 0.5F));

    // Tolerance RELATIVE : convient aux grandeurs couvrant plusieurs ordres
    // de grandeur.
    CHECK(mod15::close_relative(1000000.0F, 1000100.0F, 0.001F));
    CHECK(mod15::close_relative(10.0F, 10.001F, 0.001F));
    CHECK_FALSE(mod15::close_relative(10.0F, 11.0F, 0.001F));
}

TEST_REQ(Flottant, robustesse_des_comparaisons, "LLR-DET-020,LLR-DET-021") {
    CHECK_FALSE(mod15::close_absolute(kNaN, 1.0F, 1.0F));
    CHECK_FALSE(mod15::close_absolute(1.0F, kInf, 1.0F));
    CHECK_FALSE(mod15::close_absolute(1.0F, 1.0F, -1.0F));
    CHECK_FALSE(mod15::close_relative(kNaN, kNaN, 0.1F));
    // Deux zeros exacts : la tolerance relative n'a plus de sens, on retombe
    // sur l'egalite.
    CHECK(mod15::close_relative(0.0F, 0.0F, 0.1F));
    CHECK_FALSE(mod15::close_relative(0.0F, 1.0e-30F, 0.1F));
}

// =============================================================================
//  2. Virgule fixe
// =============================================================================
TEST_REQ(VirguleFixe, representation, "LLR-FIX-010,LLR-FIX-012") {
    CHECK_EQ(Fixed::from_int(1).raw(), Fixed::kOne);
    CHECK_EQ(Fixed::from_int(0).raw(), 0);
    CHECK_EQ(Fixed::from_int(-1).raw(), -Fixed::kOne);
    CHECK_EQ(Fixed::from_int(1000).to_int(), 1000);
    CHECK_EQ(Fixed::from_int(-1000).to_int(), -1000);
}

TEST_REQ(VirguleFixe, troncature_vers_moins_l_infini, "LLR-FIX-012") {
    // Choix SPECIFIE : le decalage arithmetique tronque vers moins l'infini.
    // -1,5 donne -2, pas -1. Un comportement non specifie ici serait un
    // constat de revue.
    CHECK_EQ(Fixed::from_float(1.5F).to_int(), 1);
    CHECK_EQ(Fixed::from_float(-1.5F).to_int(), -2);
    CHECK_EQ(Fixed::from_float(-0.5F).to_int(), -1);
}

TEST_REQ(VirguleFixe, conversion_flottante_bornee, "LLR-FIX-011") {
    // 0,1 en Q16.16 : round(0,1 x 65536) = 6554. L'erreur vaut donc
    // 6554/65536 - 0,1 = 6,1e-6, soit MOINS que kResolution/2. Cette borne
    // est CALCULABLE A LA MAIN, avant toute execution.
    CHECK_EQ(Fixed::from_float(0.1F).raw(), 6554);
    CHECK_NEAR(static_cast<double>(Fixed::from_float(0.1F).to_float()), 0.1,
               static_cast<double>(Fixed::kResolution));

    CHECK_EQ(Fixed::from_float(0.5F).raw(), Fixed::kOne / 2);   // exact
    CHECK_EQ(Fixed::from_float(0.25F).raw(), Fixed::kOne / 4);  // exact
}

TEST_REQ(VirguleFixe, robustesse_non_fini, "LLR-FIX-011") {
    CHECK_EQ(Fixed::from_float(kNaN).raw(), 0);
    CHECK_EQ(Fixed::from_float(kInf).raw(), Fixed::kRawMax);
    CHECK_EQ(Fixed::from_float(-kInf).raw(), Fixed::kRawMin);
}

TEST_REQ(VirguleFixe, arithmetique_exacte, "LLR-FIX-020,LLR-FIX-021,LLR-FIX-022") {
    const Fixed two = Fixed::from_int(2);
    const Fixed three = Fixed::from_int(3);

    // Egalite EXACTE : impensable en flottant, naturelle ici.
    CHECK(((two + three) == Fixed::from_int(5)));
    CHECK(((three - two) == Fixed::from_int(1)));
    CHECK(((two * three) == Fixed::from_int(6)));
    CHECK(((-two) == Fixed::from_int(-2)));

    const Fixed half = Fixed::from_float(0.5F);
    CHECK(((half * two) == Fixed::from_int(1)));
    CHECK(((half + half) == Fixed::from_int(1)));
}

TEST_REQ(VirguleFixe, saturation, "LLR-FIX-020,LLR-FIX-021") {
    const Fixed maximum = Fixed::from_raw(Fixed::kRawMax);
    const Fixed minimum = Fixed::from_raw(Fixed::kRawMin);
    const Fixed un = Fixed::from_int(1);

    CHECK_EQ((maximum + un).raw(), Fixed::kRawMax);
    CHECK_EQ((minimum - un).raw(), Fixed::kRawMin);
    // Domaine Q16.16 : +/- 32768. Au-dela, saturation.
    CHECK_EQ(Fixed::from_int(100000).raw(), Fixed::kRawMax);
    CHECK_EQ(Fixed::from_int(-100000).raw(), Fixed::kRawMin);
}

TEST_REQ(VirguleFixe, division, "LLR-FIX-023") {
    Fixed result;
    REQUIRE(Fixed::from_int(6).divide(Fixed::from_int(2), result));
    CHECK((result == Fixed::from_int(3)));

    REQUIRE(Fixed::from_int(1).divide(Fixed::from_int(4), result));
    CHECK((result == Fixed::from_float(0.25F)));

    // Division par zero : refusee, sortie neutralisee, jamais de piege
    // materiel.
    CHECK_FALSE(Fixed::from_int(1).divide(Fixed(), result));
    CHECK_EQ(result.raw(), 0);
}

TEST_REQ(VirguleFixe, derive_previsible, "LLR-FIX-030") {
    // LE point du module. Accumuler 1000 fois 0,1 :
    //
    //   * en FLOTTANT : le resultat depend de l'ordre des operations, du jeu
    //     d'options du compilateur et de la cible. Borner l'erreur demande une
    //     analyse numerique.
    //
    //   * en VIRGULE FIXE : le resultat vaut EXACTEMENT 1000 x 6554 = 6554000
    //     en representation interne, soit 100,006103515625. Calculable a la
    //     main, identique sur toute cible, verifiable par un simple entier.
    const Fixed increment = Fixed::from_float(0.1F);
    const Fixed total = mod15::accumulate(increment, 1000U);

    CHECK_EQ(increment.raw(), 6554);
    CHECK_EQ(total.raw(), 6554 * 1000);
    CHECK_NEAR(static_cast<double>(total.to_float()), 100.006103515625, 1e-4);

    // La virgule fixe n'est pas forcement PLUS PRECISE que le flottant :
    // elle est PREVISIBLE. C'est cela que l'on achete.
    CHECK((total == Fixed::from_raw(6554000)));
}

// =============================================================================
//  3. Ordonnancement a fenetres fixes
// =============================================================================
namespace {

/// Trame majeure de reference : 10 ms (100 Hz), quatre partitions.
MajorFrame reference_frame() noexcept {
    MajorFrame frame(10000U);
    (void)frame.add_window(PartitionId::FlightControl, 0U, 3000U);
    (void)frame.add_window(PartitionId::FuelManagement, 3000U, 1500U);
    (void)frame.add_window(PartitionId::Display, 4500U, 1500U);
    (void)frame.add_window(PartitionId::Maintenance, 6000U, 1000U);
    return frame;
}

}  // namespace

TEST_REQ(Ordonnancement, plan_valide, "LLR-SCH-010,LLR-SCH-011") {
    const MajorFrame frame = reference_frame();
    CHECK_EQ(frame.window_count(), usize{4});
    CHECK_EQ(frame.period_us(), u32{10000});
    CHECK_EQ(frame.allocated_us(), u32{7000});
    CHECK_EQ(frame.utilisation_percent(), u32{70});
    CHECK_EQ(frame.slack_us(), u32{3000});
}

TEST_REQ(Ordonnancement, chevauchement_refuse, "LLR-SCH-010") {
    // LA propriete qui garantit le partitionnement temporel. Deux fenetres
    // qui se recouvrent, et la separation entre niveaux DAL disparait.
    MajorFrame frame(10000U);
    REQUIRE(frame.add_window(PartitionId::FlightControl, 0U, 3000U));

    CHECK_FALSE(frame.add_window(PartitionId::Display, 2999U, 100U));   // debut dedans
    CHECK_FALSE(frame.add_window(PartitionId::Display, 0U, 100U));      // meme debut
    CHECK_FALSE(frame.add_window(PartitionId::Display, 1000U, 500U));   // entierement dedans
    CHECK_FALSE(frame.add_window(PartitionId::Display, 2000U, 2000U));  // a cheval sur la fin

    // Juste apres la fin : accepte.
    CHECK(frame.add_window(PartitionId::Display, 3000U, 1000U));
    CHECK_EQ(frame.window_count(), usize{2});
}

TEST_REQ(Ordonnancement, fenetre_hors_periode_refusee, "LLR-SCH-010") {
    MajorFrame frame(10000U);
    CHECK_FALSE(frame.add_window(PartitionId::FlightControl, 9500U, 1000U));  // deborde
    CHECK_FALSE(frame.add_window(PartitionId::FlightControl, 10000U, 1U));    // hors periode
    CHECK(frame.add_window(PartitionId::FlightControl, 9000U, 1000U));        // pile a la fin
}

TEST_REQ(Ordonnancement, fenetre_degeneree_refusee, "LLR-SCH-010") {
    MajorFrame frame(10000U);
    CHECK_FALSE(frame.add_window(PartitionId::FlightControl, 0U, 0U));
    CHECK_FALSE(frame.add_window(PartitionId::Count, 0U, 100U));
    CHECK_EQ(frame.window_count(), usize{0});
}

TEST_REQ(Ordonnancement, capacite_bornee, "LLR-SCH-010") {
    MajorFrame frame(100000U);
    for (usize index = 0U; index < MajorFrame::kMaxWindows; ++index) {
        REQUIRE(frame.add_window(PartitionId::Display, static_cast<u32>(index) * 1000U, 500U));
    }
    CHECK_FALSE(frame.add_window(PartitionId::Display, 50000U, 500U));
    CHECK_EQ(frame.window_count(), MajorFrame::kMaxWindows);
}

TEST_REQ(Ordonnancement, marge_temporelle, "LLR-SCH-012") {
    const MajorFrame frame = reference_frame();
    // 30 % de marge : conforme a une exigence de 20 %, pas a une exigence
    // de 40 %.
    CHECK(frame.has_margin(20U));
    CHECK(frame.has_margin(30U));
    CHECK_FALSE(frame.has_margin(40U));

    // Une trame allouee a 100 % n'a AUCUNE marge : la moindre variation
    // (defaut de cache, interruption materielle) provoque un depassement.
    MajorFrame saturated(10000U);
    REQUIRE(saturated.add_window(PartitionId::FlightControl, 0U, 10000U));
    CHECK_EQ(saturated.utilisation_percent(), u32{100});
    CHECK_FALSE(saturated.has_margin(1U));
}

TEST_REQ(Ordonnancement, execution_nominale, "LLR-SCH-020") {
    const MajorFrame frame = reference_frame();
    const u32 consumptions[4] = {2500U, 1200U, 1400U, 800U};

    const mod15::FrameResult result = mod15::run_major_frame(frame, consumptions, 4U);
    CHECK(result.deadline_met);
    CHECK_EQ(result.overrun_count, usize{0});
    CHECK_EQ(result.worst_overrun_us, u32{0});
}

TEST_REQ(Ordonnancement, depassement_detecte_et_localise, "LLR-SCH-020") {
    const MajorFrame frame = reference_frame();
    // La partition Maintenance (DAL D) deborde de 500 us.
    const u32 consumptions[4] = {2500U, 1200U, 1400U, 1500U};

    const mod15::FrameResult result = mod15::run_major_frame(frame, consumptions, 4U);
    CHECK_FALSE(result.deadline_met);
    CHECK_EQ(result.overrun_count, usize{1});
    CHECK_EQ(result.worst_overrun_us, u32{500});
    CHECK_EQ(result.worst_partition, PartitionId::Maintenance);

    // POINT CLE : la partition fautive est IDENTIFIEE, et les partitions
    // suivantes ne sont pas affectees. Sans partitionnement temporel, une
    // fonction DAL D pourrait retarder une fonction DAL A.
}

TEST_REQ(Ordonnancement, plusieurs_depassements, "LLR-SCH-020") {
    const MajorFrame frame = reference_frame();
    const u32 consumptions[4] = {3500U, 1200U, 3000U, 800U};

    const mod15::FrameResult result = mod15::run_major_frame(frame, consumptions, 4U);
    CHECK_EQ(result.overrun_count, usize{2});
    // Le PIRE depassement est retenu : 1500 us pour Display, contre 500 pour
    // FlightControl.
    CHECK_EQ(result.worst_overrun_us, u32{1500});
    CHECK_EQ(result.worst_partition, PartitionId::Display);
}

TEST_REQ(Ordonnancement, robustesse_entrees, "LLR-SCH-020") {
    const MajorFrame frame = reference_frame();
    const mod15::FrameResult without_data = mod15::run_major_frame(frame, nullptr, 4U);
    CHECK(without_data.deadline_met);
    CHECK_EQ(without_data.overrun_count, usize{0});

    // Moins de mesures que de fenetres : on n'analyse que ce que l'on a.
    const u32 partial_durations[2] = {5000U, 100U};
    const mod15::FrameResult partial_result = mod15::run_major_frame(frame, partial_durations, 2U);
    CHECK_EQ(partial_result.overrun_count, usize{1});
}

// =============================================================================
//  4. Registre materiel et attente bornee
// =============================================================================
TEST_REQ(Registre, attente_reussie, "LLR-DET-030") {
    mod15::SimulatedRegister hw_register;
    hw_register.set_hardware_value(0x0004U);
    CHECK(mod15::wait_for_bit(hw_register, 0x0004U, 100U));
    CHECK_EQ(hw_register.read_count(), u32{1});
}

TEST_REQ(Registre, attente_bornee_en_cas_de_panne, "LLR-DET-030") {
    // Le materiel ne repond jamais. La boucle DOIT se terminer : une attente
    // active non bornee provoquerait un redemarrage par le chien de garde,
    // evenement bien plus grave en vol que la panne du peripherique.
    mod15::SimulatedRegister hw_register;
    hw_register.set_hardware_value(0x0000U);

    CHECK_FALSE(mod15::wait_for_bit(hw_register, 0x0004U, 50U));
    CHECK_EQ(hw_register.read_count(), u32{50});  // exactement le budget, pas plus
}

TEST_REQ(Registre, robustesse_masque_nul, "LLR-DET-030") {
    mod15::SimulatedRegister hw_register;
    hw_register.set_hardware_value(0xFFFFFFFFU);
    CHECK_FALSE(mod15::wait_for_bit(hw_register, 0U, 10U));
    CHECK_EQ(hw_register.read_count(), u32{0});  // aucun acces inutile
}

TEST_REQ(Registre, ecriture_et_relecture, "LLR-DET-030") {
    mod15::SimulatedRegister hw_register;
    hw_register.write(0x1234U);
    CHECK_EQ(hw_register.read(), u32{0x1234U});
}
