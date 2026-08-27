// =============================================================================
//  Module 15 -- ordonnancement a fenetres fixes (facon ARINC 653).
//
//  L'ARINC 653 definit l'interface d'un noyau temps reel partitionne, utilise
//  par la quasi-totalite de l'avionique modulaire (IMA). Son principe est
//  simple et radical :
//
//    PARTITIONNEMENT SPATIAL  : chaque partition dispose de sa propre memoire,
//                               protegee par la MMU. Une partition ne peut PAS
//                               corrompre une autre.
//    PARTITIONNEMENT TEMPOREL : chaque partition dispose de FENETRES FIXES,
//                               definies hors ligne. Une partition qui deborde
//                               est interrompue ; elle ne peut PAS voler du
//                               temps a une autre.
//
//  C'est cela, la "freedom from interference" : une fonction DAL D ne peut ni
//  corrompre la memoire, ni retarder l'execution d'une fonction DAL A qui
//  partage le meme calculateur. Sans cela, tout le calculateur devrait etre
//  developpe au niveau le plus severe.
//
//  Consequence de conception : l'ordonnancement n'est PAS dynamique. Il n'y a
//  ni priorites globales, ni preemption entre partitions, ni file d'attente.
//  Le plan d'execution -- la MAJOR FRAME -- est une donnee de configuration,
//  fixee a la conception et verifiee hors ligne.
//
//  Ce module implemente la validation de ce plan et la detection des
//  depassements de budget.
// =============================================================================
#ifndef MOD15_SCHEDULE_HPP
#define MOD15_SCHEDULE_HPP

#include <avio/types.hpp>

namespace mod15 {

/// Identifiant de partition.
enum class PartitionId : avio::u8 {
    FlightControl = 0U,   ///< DAL A
    FuelManagement = 1U,  ///< DAL B
    Maintenance = 2U,     ///< DAL D
    Display = 3U,         ///< DAL C
    Count = 4U
};

const char* partition_name(PartitionId partition) noexcept;

/// Une fenetre d'execution dans la major frame.
struct Window {
    PartitionId partition = PartitionId::Count;
    avio::u32 offset_us = 0U;    ///< debut, en microsecondes depuis le debut de trame
    avio::u32 duration_us = 0U;  ///< budget alloue
};

/// Plan d'execution cyclique.
class MajorFrame {
public:
    static constexpr avio::usize kMaxWindows = 8U;

    /// @param period_us duree de la trame majeure (par exemple 10000 us = 100 Hz)
    explicit MajorFrame(avio::u32 period_us) noexcept;

    /// Ajoute une fenetre.
    /// @satisfies LLR-SCH-010
    /// @return false si la fenetre est vide, sort de la periode, chevauche une
    ///         fenetre existante, ou si la capacite est atteinte
    bool add_window(PartitionId partition, avio::u32 offset_us, avio::u32 duration_us) noexcept;

    avio::u32 period_us() const noexcept { return period_us_; }
    avio::usize window_count() const noexcept { return count_; }
    const Window& window(avio::usize index) const noexcept;

    /// Somme des budgets alloues, en microsecondes.
    /// @satisfies LLR-SCH-011
    avio::u32 allocated_us() const noexcept;

    /// Taux d'occupation en pour-cent (arrondi a l'entier inferieur).
    /// @satisfies LLR-SCH-011
    avio::u32 utilisation_percent() const noexcept;

    /// Temps non alloue, disponible pour la marge.
    avio::u32 slack_us() const noexcept;

    /// Le plan respecte-t-il la marge minimale exigee ?
    ///
    /// Une trame allouee a 100 % n'a AUCUNE marge : la moindre variation
    /// (defaut de cache, interruption materielle) provoque un depassement.
    /// Les programmes reels exigent typiquement 20 a 30 % de marge.
    /// @satisfies LLR-SCH-012
    bool has_margin(avio::u32 minimum_slack_percent) const noexcept;

private:
    Window windows_[kMaxWindows] = {};
    avio::usize count_ = 0U;
    avio::u32 period_us_ = 0U;
};

/// Resultat de l'execution simulee d'une trame majeure.
struct FrameResult {
    avio::usize overrun_count = 0U;   ///< nombre de fenetres en depassement
    avio::u32 worst_overrun_us = 0U;  ///< pire depassement observe
    PartitionId worst_partition = PartitionId::Count;
    bool deadline_met = true;  ///< aucune fenetre n'a deborde
};

/// Simule l'execution d'une trame majeure.
///
/// @param frame           le plan
/// @param execution_us    temps reellement consomme par chaque fenetre, dans
///                        l'ordre des fenetres du plan
/// @param count           nombre d'elements du tableau
///
/// Une partition qui deborde est INTERROMPUE a la fin de sa fenetre : elle ne
/// vole pas de temps aux suivantes. Le depassement est enregistre et remonte
/// au systeme de sante (health monitoring, en vocabulaire ARINC 653).
/// @satisfies LLR-SCH-020
FrameResult run_major_frame(const MajorFrame& frame, const avio::u32* execution_us,
                            avio::usize count) noexcept;

}  // namespace mod15

#endif  // MOD15_SCHEDULE_HPP
