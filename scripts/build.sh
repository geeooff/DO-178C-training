#!/usr/bin/env bash
#
# build.sh -- configure, compile et teste la formation sous Linux et macOS.
#
# Equivalent POSIX de scripts/build.ps1. Les deux scripts acceptent les memes
# presets et produisent le meme resultat : c'est volontaire. Un projet dont le
# comportement depend de la machine de celui qui le compile n'est pas
# reproductible -- et la reproductibilite est une exigence de la DO-178C
# (section 7, gestion de configuration).
#
# USAGE
#   ./scripts/build.sh                       # configure + compile (debug)
#   ./scripts/build.sh -t                    # + lance les tests
#   ./scripts/build.sh -p strict -t          # warnings = erreurs + clang-tidy
#   ./scripts/build.sh -p asan -t            # sanitizers
#   ./scripts/build.sh -p gcc-strict -t      # force GCC
#   ./scripts/build.sh -p clang-strict -t    # force Clang
#   ./scripts/build.sh -c                    # supprime build/ et repart de zero
#   ./scripts/build.sh -l                    # liste les presets disponibles

set -o errexit
set -o nounset
set -o pipefail

PRESET="debug"
RUN_TESTS=0
CLEAN=0
LIST=0

# --- Couleurs, seulement si la sortie est un terminal -------------------------
if [ -t 1 ]; then
    C_INFO=$'\033[36m'; C_OK=$'\033[32m'; C_WARN=$'\033[33m'
    C_ERR=$'\033[31m';  C_DIM=$'\033[2m';  C_OFF=$'\033[0m'
else
    C_INFO=""; C_OK=""; C_WARN=""; C_ERR=""; C_DIM=""; C_OFF=""
fi

usage() {
    cat <<'FIN'
build.sh -- configure, compile et teste la formation sous Linux et macOS.

  -p <preset>   preset CMake (defaut : debug)
                debug | release | strict | asan | coverage
                gcc-strict | clang-strict
  -t            lance les tests apres la compilation
  -c            supprime build/ avant de configurer
  -l            liste les presets disponibles sur cette machine
  -h            affiche cette aide

Exemples :
  ./scripts/build.sh -t
  ./scripts/build.sh -p strict -t
  ./scripts/build.sh -p asan -t
FIN
}

erreur() {
    printf '%sERREUR : %s%s\n' "$C_ERR" "$1" "$C_OFF" >&2
    exit 1
}

while getopts ":p:tclh" opt; do
    case "$opt" in
        p) PRESET="$OPTARG" ;;
        t) RUN_TESTS=1 ;;
        c) CLEAN=1 ;;
        l) LIST=1 ;;
        h) usage; exit 0 ;;
        \?) erreur "option inconnue : -$OPTARG (utilisez -h)" ;;
        :)  erreur "l'option -$OPTARG attend une valeur" ;;
    esac
done

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

# --- 1. Verification de l'outillage -------------------------------------------
#
# On echoue TOT et avec un message actionnable. Un script qui part en vrille
# trois minutes plus tard parce qu'un outil manquait est un script hostile.

detecter_gestionnaire() {
    if   command -v apt-get >/dev/null 2>&1; then echo "sudo apt-get install -y"
    elif command -v dnf     >/dev/null 2>&1; then echo "sudo dnf install -y"
    elif command -v pacman  >/dev/null 2>&1; then echo "sudo pacman -S --noconfirm"
    elif command -v zypper  >/dev/null 2>&1; then echo "sudo zypper install -y"
    elif command -v brew    >/dev/null 2>&1; then echo "brew install"
    else echo ""
    fi
}

exiger() {
    local outil="$1" paquet="$2"
    if ! command -v "$outil" >/dev/null 2>&1; then
        local gest; gest="$(detecter_gestionnaire)"
        printf '%s%s introuvable.%s\n' "$C_ERR" "$outil" "$C_OFF" >&2
        if [ -n "$gest" ]; then
            printf '  Installation : %s%s %s%s\n' "$C_INFO" "$gest" "$paquet" "$C_OFF" >&2
        fi
        exit 1
    fi
}

exiger cmake cmake
exiger ninja ninja-build

if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
    gest="$(detecter_gestionnaire)"
    printf '%sAucun compilateur C++ trouve (g++ ou clang++).%s\n' "$C_ERR" "$C_OFF" >&2
    [ -n "$gest" ] && printf '  Installation : %s%s g++%s\n' "$C_INFO" "$gest" "$C_OFF" >&2
    exit 1
fi

if [ "$LIST" -eq 1 ]; then
    printf '%s=== Presets disponibles sur cette machine ===%s\n' "$C_INFO" "$C_OFF"
    cmake --list-presets
    exit 0
fi

# --- 2. Nettoyage -------------------------------------------------------------
if [ "$CLEAN" -eq 1 ]; then
    printf '%sSuppression de %s/build%s\n' "$C_WARN" "$REPO_ROOT" "$C_OFF"
    rm -rf "$REPO_ROOT/build"
fi

# --- 3. Environnement ---------------------------------------------------------
printf '%sSysteme    :%s %s %s\n' "$C_INFO" "$C_OFF" "$(uname -s)" "$(uname -m)"
if command -v g++ >/dev/null 2>&1; then
    printf '%sg++        :%s %s\n' "$C_INFO" "$C_OFF" "$(g++ --version | head -1)"
fi
if command -v clang++ >/dev/null 2>&1; then
    printf '%sclang++    :%s %s\n' "$C_INFO" "$C_OFF" "$(clang++ --version | head -1)"
fi
printf '%scmake      :%s %s\n' "$C_INFO" "$C_OFF" "$(cmake --version | head -1)"
printf '%sninja      :%s %s\n' "$C_INFO" "$C_OFF" "$(ninja --version)"
if command -v clang-tidy >/dev/null 2>&1; then
    printf '%sclang-tidy :%s %s\n' "$C_INFO" "$C_OFF" "$(clang-tidy --version | sed -n 2p | sed 's/^ *//')"
else
    printf '%sclang-tidy :%s absent -- le preset strict compilera sans analyse statique\n' \
        "$C_DIM" "$C_OFF"
fi

# --- 4. Configure / Build / Test ---------------------------------------------
printf '\n%s=== Configuration (%s) ===%s\n' "$C_OK" "$PRESET" "$C_OFF"
cmake --preset "$PRESET"

printf '\n%s=== Compilation (%s) ===%s\n' "$C_OK" "$PRESET" "$C_OFF"
# -k 0 : ne s'arrete pas a la premiere erreur. Sur un build avec
# warnings = erreurs, cela donne la LISTE COMPLETE des problemes en une passe
# plutot qu'un aller-retour par fichier.
cmake --build --preset "$PRESET" -- -k 0

if [ "$RUN_TESTS" -eq 1 ]; then
    printf '\n%s=== Tests (%s) ===%s\n' "$C_OK" "$PRESET" "$C_OFF"
    ctest --preset "$PRESET"
fi

printf '\n%sTermine.%s\n' "$C_OK" "$C_OFF"
