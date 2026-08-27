#include "mod11/mcdc.hpp"

namespace mod11 {

DecisionRecorder::DecisionRecorder(avio::usize condition_count) noexcept
    : condition_count_((condition_count <= kMaxConditions) ? condition_count : kMaxConditions) {}

void DecisionRecorder::reset() noexcept {
    count_ = 0U;
}

bool DecisionRecorder::record(const bool* conditions, avio::usize count, bool outcome) noexcept {
    if ((conditions == nullptr) || (count != condition_count_) || (count_ >= kMaxEvaluations)) {
        return false;
    }
    for (avio::usize index = 0U; index < count; ++index) {
        evaluations_[count_].conditions[index] = conditions[index];
    }
    evaluations_[count_].outcome = outcome;
    count_ += 1U;
    return true;
}

const Evaluation& DecisionRecorder::at(avio::usize index) const noexcept {
    // Robustesse : un index hors domaine renvoie la premiere evaluation
    // plutot qu'une lecture sauvage.
    return evaluations_[(index < count_) ? index : 0U];
}

avio::usize McdcReport::covered_count() const noexcept {
    avio::usize total = 0U;
    for (avio::usize index = 0U; index < condition_count; ++index) {
        if (condition_covered[index]) {
            total += 1U;
        }
    }
    return total;
}

bool McdcReport::is_complete() const noexcept {
    if (!outcome_true_seen || !outcome_false_seen) {
        return false;
    }
    for (avio::usize index = 0U; index < condition_count; ++index) {
        if (!condition_covered[index] || !condition_both_values[index]) {
            return false;
        }
    }
    return true;
}

/// @satisfies LLR-GPWS-050
McdcReport analyze_mcdc(const DecisionRecorder& recorder) noexcept {
    McdcReport report;
    report.condition_count = recorder.condition_count();

    const avio::usize evaluations = recorder.size();
    const avio::usize conditions = report.condition_count;

    // --- Critere 3 : la decision a-t-elle pris ses deux issues ? -------------
    for (avio::usize index = 0U; index < evaluations; ++index) {
        if (recorder.at(index).outcome) {
            report.outcome_true_seen = true;
        } else {
            report.outcome_false_seen = true;
        }
    }

    // --- Critere 2 : chaque condition a-t-elle pris ses deux valeurs ? -------
    for (avio::usize condition = 0U; condition < conditions; ++condition) {
        bool true_seen = false;
        bool false_seen = false;
        for (avio::usize index = 0U; index < evaluations; ++index) {
            if (recorder.at(index).conditions[condition]) {
                true_seen = true;
            } else {
                false_seen = true;
            }
        }
        report.condition_both_values[condition] = true_seen && false_seen;
    }

    // --- Critere 4 : paire d'independance ("unique cause") -------------------
    //
    // Pour chaque condition C, on cherche deux evaluations E1 et E2 telles que :
    //     * E1 et E2 different sur C ;
    //     * E1 et E2 sont IDENTIQUES sur toutes les autres conditions ;
    //     * l'issue de la decision differe entre E1 et E2.
    //
    // Une telle paire demontre que C affecte SEULE l'issue : c'est exactement
    // ce que demande la definition.
    for (avio::usize condition = 0U; condition < conditions; ++condition) {
        for (avio::usize first = 0U; (first < evaluations) && !report.condition_covered[condition];
             ++first) {
            for (avio::usize second = first + 1U; second < evaluations; ++second) {
                const Evaluation& a = recorder.at(first);
                const Evaluation& b = recorder.at(second);

                if (a.outcome == b.outcome) {
                    continue;  // l'issue doit changer
                }
                if (a.conditions[condition] == b.conditions[condition]) {
                    continue;  // la condition etudiee doit changer
                }

                bool others_identical = true;
                for (avio::usize other_index = 0U; other_index < conditions; ++other_index) {
                    if (other_index == condition) {
                        continue;
                    }
                    if (a.conditions[other_index] != b.conditions[other_index]) {
                        others_identical = false;
                        break;
                    }
                }

                if (others_identical) {
                    report.condition_covered[condition] = true;
                    report.pair_first[condition] = first;
                    report.pair_second[condition] = second;
                    break;
                }
            }
        }
    }

    return report;
}

}  // namespace mod11
