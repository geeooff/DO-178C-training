#!/usr/bin/env bash
#
# coverage.sh -- couverture d'instructions et de branches sous Linux et macOS.
#
# Equivalent POSIX de scripts/coverage.ps1, avec une difference notable :
# gcov et llvm-cov mesurent la couverture d'instructions ET DE BRANCHES, la ou
# OpenCppCoverage (Windows) ne mesure que les instructions.
#
#   couverture de BRANCHES  ~  couverture de DECISION (objectif A-7.6, DAL A/B)
#
# La correspondance n'est pas exacte : gcov compte les branches du code GENERE,
# pas les decisions du code SOURCE. Un `&&` a court-circuit produit plusieurs
# branches pour une seule decision. C'est utilisable comme INDICATEUR, jamais
# comme preuve de conformite (module 11, section 1.8).
#
# Et dans tous les cas : NI gcov NI llvm-cov ne mesurent le MC/DC.
#
# USAGE
#   ./scripts/coverage.sh                 # tout le depot
#   ./scripts/coverage.sh -m 16-projet-integre
#   ./scripts/coverage.sh -o rapports/couv

set -o errexit
set -o nounset
set -o pipefail

MODULE=""
OUTPUT="reports/coverage"
PRESET="coverage"

if [ -t 1 ]; then
    C_INFO=$'\033[36m'; C_OK=$'\033[32m'; C_WARN=$'\033[33m'; C_OFF=$'\033[0m'
else
    C_INFO=""; C_OK=""; C_WARN=""; C_OFF=""
fi

while getopts ":m:o:h" opt; do
    case "$opt" in
        m) MODULE="$OPTARG" ;;
        o) OUTPUT="$OPTARG" ;;
        h) sed -n '3,24p' "$0" | sed 's/^#\{0,1\} \{0,1\}//'; exit 0 ;;
        *) printf 'Option inconnue. Utilisez -h.\n' >&2; exit 1 ;;
    esac
done

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

# --- 1. Construire avec instrumentation --------------------------------------
printf '%s=== Construction instrumentee ===%s\n' "$C_OK" "$C_OFF"
cmake --preset "$PRESET"
cmake --build --preset "$PRESET" -- -k 0

# --- 2. Executer les tests ----------------------------------------------------
printf '\n%s=== Execution des tests ===%s\n' "$C_OK" "$C_OFF"
if [ -n "$MODULE" ]; then
    ctest --preset "$PRESET" -R "$MODULE" --output-on-failure
else
    ctest --preset "$PRESET" --output-on-failure
fi

BUILD_DIR="$REPO_ROOT/build/$PRESET"
mkdir -p "$OUTPUT"

# --- 3. Produire le rapport ---------------------------------------------------
#
# Trois outils possibles, par ordre de preference :
#   gcovr  -- le plus simple, produit du HTML directement, gere GCC et Clang
#   lcov   -- classique, HTML via genhtml
#   gcov   -- toujours present, sortie texte brute

if command -v gcovr >/dev/null 2>&1; then
    printf '\n%s=== Rapport (gcovr) ===%s\n' "$C_OK" "$C_OFF"
    # --exclude : on mesure la couverture du code de PRODUCTION, pas celle du
    # harnais de test ni du code volontairement non conforme du module 13.
    gcovr \
        --root "$REPO_ROOT" \
        --filter "$REPO_ROOT/modules/" \
        --filter "$REPO_ROOT/common/" \
        --exclude '.*/tests/.*' \
        --exclude '.*/src/main\.cpp' \
        --exclude '.*nonconforming\.cpp' \
        --exclude '.*coupling_trace\.cpp' \
        --html-details "$OUTPUT/index.html" \
        --txt \
        --print-summary \
        "$BUILD_DIR"
    printf '\n%sRapport HTML : %s/index.html%s\n' "$C_OK" "$OUTPUT" "$C_OFF"

elif command -v lcov >/dev/null 2>&1 && command -v genhtml >/dev/null 2>&1; then
    printf '\n%s=== Rapport (lcov) ===%s\n' "$C_OK" "$C_OFF"
    lcov --capture --directory "$BUILD_DIR" --output-file "$OUTPUT/brut.info" \
         --rc branch_coverage=1 --quiet
    lcov --remove "$OUTPUT/brut.info" \
         '*/tests/*' '*/src/main.cpp' '*nonconforming.cpp' '*coupling_trace.cpp' \
         '/usr/*' \
         --output-file "$OUTPUT/filtre.info" --rc branch_coverage=1 --quiet
    genhtml "$OUTPUT/filtre.info" --output-directory "$OUTPUT" \
            --branch-coverage --quiet
    printf '\n%sRapport HTML : %s/index.html%s\n' "$C_OK" "$OUTPUT" "$C_OFF"

else
    printf '\n%sNi gcovr ni lcov ne sont installes.%s\n' "$C_WARN" "$C_OFF"
    if command -v apt-get >/dev/null 2>&1; then
        printf '  Installation : %ssudo apt-get install -y gcovr%s\n' "$C_INFO" "$C_OFF"
    elif command -v brew >/dev/null 2>&1; then
        printf '  Installation : %sbrew install gcovr%s\n' "$C_INFO" "$C_OFF"
    fi
    exit 1
fi

printf '\n%sRAPPEL :%s gcov mesure les instructions et les BRANCHES DU CODE GENERE.\n' "$C_WARN" "$C_OFF"
printf '         Ce n%sest PAS la couverture de decision au sens DO-178C, et ce\n' "'"
printf '         n%sest en aucun cas du MC/DC. Voir module 11, section 1.8.\n' "'"
