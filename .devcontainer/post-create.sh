#!/usr/bin/env bash
#
# Execute une fois, a la creation du conteneur.
#
# On ne fait ici que ce qui est REPRODUCTIBLE et sans surprise : marquer le
# depot comme sur, rendre les scripts executables, et afficher l'environnement.
# On ne compile PAS automatiquement : la premiere compilation appartient a
# l'utilisateur, et elle fait partie du module 00.

set -o errexit
set -o nounset

# Le montage vient de l'hote : Git refuserait d'operer sur un depot dont le
# proprietaire differe. C'est une protection legitime, qu'on leve explicitement
# pour ce chemin precis.
git config --global --add safe.directory /workspace 2>/dev/null || true

chmod +x scripts/*.sh 2>/dev/null || true

cat <<'FIN'

  ============================================================
   Environnement de formation C++ / DO-178C
  ============================================================

  Chaine de production :
FIN

printf '    %-12s %s\n' "systeme"  "$(. /etc/os-release && echo "$PRETTY_NAME") $(uname -m)"
printf '    %-12s %s\n' "g++"      "$(g++ --version | head -1)"
printf '    %-12s %s\n' "clang++"  "$(clang++ --version | head -1)"
printf '    %-12s %s\n' "cmake"    "$(cmake --version | head -1)"
printf '    %-12s %s\n' "ninja"    "$(ninja --version)"
printf '    %-12s %s\n' "clang-tidy" "$(clang-tidy --version | sed -n 2p | sed 's/^ *//')"
printf '    %-12s %s\n' "gcovr"    "$(gcovr --version | head -1)"
printf '    %-12s %s\n' "python3"  "$(python3 --version)"

cat <<'FIN'

  Pour demarrer :

    ./scripts/build.sh -t                 compile et teste (debug)
    ./scripts/build.sh -p strict -t       warnings = erreurs + clang-tidy
    ./scripts/build.sh -p asan -t         sanitizers (ASan + UBSan)
    ./scripts/coverage.sh                 couverture instructions et branches

    python3 tools/trace_check.py          matrice de tracabilite
    python3 tools/config_index.py         SCI et SECI

  Le cours commence ici : modules/00-environnement/README.md

FIN
