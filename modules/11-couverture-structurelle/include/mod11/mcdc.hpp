// =============================================================================
//  Module 11 -- analyseur MC/DC.
//
//  MC/DC = Modified Condition / Decision Coverage. Exige par la DO-178C au
//  seul niveau DAL A (objectif A-7.5). C'est le critere de couverture le plus
//  souvent mal compris du domaine ; cet outil le rend tangible.
//
//  DEFINITION (DO-178C, section 6.4.4.2 et glossaire)
//  --------------------------------------------------
//  La couverture MC/DC est atteinte lorsque :
//    (1) chaque POINT D'ENTREE et de sortie du programme a ete invoque ;
//    (2) chaque CONDITION d'une decision a pris toutes les valeurs possibles ;
//    (3) chaque DECISION a pris toutes les issues possibles ;
//    (4) chaque CONDITION d'une decision a demontre qu'elle affecte SEULE
//        l'issue de cette decision.
//
//  C'est le point (4) qui distingue MC/DC de la simple couverture des
//  conditions. Il demande, pour CHAQUE condition, d'exhiber une PAIRE
//  D'INDEPENDANCE : deux jeux d'entrees ou seule cette condition change, et
//  ou l'issue de la decision change aussi.
//
//  COMBIEN DE TESTS ?
//  ------------------
//  Pour N conditions : au minimum N+1 tests, au maximum 2^N. Le gain par
//  rapport a la couverture exhaustive est enorme des que N grandit :
//    N = 4  ->  5 tests au lieu de 16
//    N = 8  ->  9 tests au lieu de 256
//    N = 16 -> 17 tests au lieu de 65 536
//  C'est precisement ce compromis qui a fait retenir MC/DC pour le DAL A.
//
//  VARIANTE RETENUE : "unique cause" MC/DC
//  ---------------------------------------
//  Deux variantes sont acceptees par les autorites :
//    * UNIQUE CAUSE : la paire d'independance ne differe QUE par la condition
//      etudiee. C'est la definition d'origine, la plus stricte, celle
//      implementee ici.
//    * MASKING : on autorise d'autres conditions a changer, a condition de
//      demontrer qu'elles sont MASQUEES (sans effet sur l'issue). Necessaire
//      des qu'une decision contient des conditions couplees.
//
//  IMPORTANT : cet analyseur travaille sur des evaluations OU TOUTES LES
//  CONDITIONS SONT EVALUEES. En C++, `&&` et `||` sont a COURT-CIRCUIT :
//  `a && b` n'evalue pas `b` si `a` est faux. Le code de production doit donc
//  evaluer les conditions dans des variables nommees AVANT de former la
//  decision -- ce qui est de toute facon une bonne pratique, et ce que fait
//  gpws.cpp.
// =============================================================================
#ifndef MOD11_MCDC_HPP
#define MOD11_MCDC_HPP

#include <avio/types.hpp>

namespace mod11 {

constexpr avio::usize kMaxConditions = 8U;
constexpr avio::usize kMaxEvaluations = 64U;

/// Une evaluation de la decision : le vecteur des conditions et l'issue.
struct Evaluation {
    bool conditions[kMaxConditions] = {};
    bool outcome = false;
};

/// Enregistreur d'evaluations. Aucune allocation dynamique.
class DecisionRecorder {
public:
    explicit DecisionRecorder(avio::usize condition_count) noexcept;

    void reset() noexcept;

    /// Enregistre une evaluation.
    /// @return false si la capacite est atteinte ou le nombre de conditions
    ///         ne correspond pas
    bool record(const bool* conditions, avio::usize count, bool outcome) noexcept;

    avio::usize size() const noexcept { return count_; }
    avio::usize condition_count() const noexcept { return condition_count_; }
    const Evaluation& at(avio::usize index) const noexcept;

private:
    Evaluation evaluations_[kMaxEvaluations] = {};
    avio::usize count_ = 0U;
    avio::usize condition_count_ = 0U;
};

/// Resultat de l'analyse MC/DC d'un jeu d'evaluations.
struct McdcReport {
    avio::usize condition_count = 0U;

    /// Pour chaque condition : une paire d'independance a-t-elle ete trouvee ?
    bool condition_covered[kMaxConditions] = {};

    /// Indices des deux evaluations formant la paire (valides si couverte).
    avio::usize pair_first[kMaxConditions] = {};
    avio::usize pair_second[kMaxConditions] = {};

    /// La decision a-t-elle pris ses deux issues ? (critere 3)
    bool outcome_true_seen = false;
    bool outcome_false_seen = false;

    /// Chaque condition a-t-elle pris ses deux valeurs ? (critere 2)
    bool condition_both_values[kMaxConditions] = {};

    /// Nombre de conditions pour lesquelles une paire a ete trouvee.
    avio::usize covered_count() const noexcept;

    /// Vrai si les criteres 2, 3 et 4 sont satisfaits pour toutes les conditions.
    bool is_complete() const noexcept;
};

/// Analyse un jeu d'evaluations et produit le rapport MC/DC.
McdcReport analyze_mcdc(const DecisionRecorder& recorder) noexcept;

}  // namespace mod11

#endif  // MOD11_MCDC_HPP
