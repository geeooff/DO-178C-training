# =============================================================================
#  Environnement de developpement et de verification de la formation.
#
#  POURQUOI UN CONTENEUR, DANS UNE FORMATION DO-178C ?
#  ---------------------------------------------------
#  Parce que c'est l'illustration la plus concrete du SECI -- le "Software Life
#  Cycle Environment Configuration Index" de la section 11.15, que le module 00
#  presente et que le module 14 outille.
#
#  La norme exige de figer et d'identifier TOUT ce qui a servi a produire et a
#  verifier le logiciel : compilateur et version exacte, options, editeur de
#  liens, systeme hote, outils de test et d'analyse. Le but n'est pas
#  bureaucratique : un aeronef vole trente a quarante ans, et il faudra
#  peut-etre corriger une anomalie en 2060 sur un binaire compile en 2026.
#
#  Ce fichier EST un SECI executable. Il repond a la question "avec quoi
#  exactement ce binaire a-t-il ete produit ?" d'une facon que dix pages de
#  document ne permettent pas : en le reconstruisant.
#
#  CE QU'IL FAUDRAIT FAIRE EN PLUS SUR UN VRAI PROGRAMME
#  ----------------------------------------------------
#    * epingler l'image de base par EMPREINTE, pas par etiquette :
#          FROM ubuntu@sha256:<64 caracteres>
#      `ubuntu:26.04` designe une image qui CHANGE (mises a jour de securite).
#      Une empreinte designe des octets. C'est la difference entre "la version
#      26.04" et "cette version-la".
#    * epingler chaque paquet a sa version exacte (apt-get install paquet=1.2.3) ;
#    * archiver l'image construite, pas seulement le Dockerfile ;
#    * placer le tout sous controle de configuration CC1.
#
#  On ne le fait pas ici : ce serait ingerable pour une formation qui doit
#  rester installable dans deux ans. Mais SAVOIR pourquoi on ne le fait pas est
#  exactement ce qu'un auditeur attend d'un ingenieur.
# =============================================================================
FROM ubuntu:26.04

# Evite toute question interactive pendant la construction : un build
# non reproductible parce qu'il attend une reponse humaine n'est pas un build.
ARG DEBIAN_FRONTEND=noninteractive

# -----------------------------------------------------------------------------
#  Chaine de production et de verification
# -----------------------------------------------------------------------------
#  build-essential  : GCC et g++
#  libclang-rt-dev  : runtimes de sanitizers de Clang. Le metapaquet `clang`
#      ne les fournit pas : sans lui, `-fsanitize=address` compile mais ne
#      s'edite pas.
#  clang, clang-tidy, clang-format : la SECONDE chaine. Compiler avec deux
#      compilateurs, c'est deux analyses statiques pour le prix d'une : chacun
#      detecte ce que l'autre laisse passer.
#  cmake, ninja-build : generation et execution du build
#  gcovr, lcov      : couverture d'instructions et de branches (module 11)
#  gdb, valgrind    : diagnostic
#  python3          : outils trace_check.py et config_index.py (module 09 et 14)
# -----------------------------------------------------------------------------
RUN apt-get update && apt-get install --no-install-recommends -y \
        build-essential \
        clang \
        libclang-rt-dev \
        clang-format \
        clang-tidy \
        cmake \
        ninja-build \
        gcovr \
        lcov \
        gdb \
        valgrind \
        git \
        ca-certificates \
        locales \
        python3 \
        python3-pip \
        less \
        nano \
    && rm -rf /var/lib/apt/lists/*

# Locale UTF-8 : les README et les sorties de programme contiennent des
# accents. Sans cela, l'affichage se degrade silencieusement.
RUN sed -i '/fr_FR.UTF-8/s/^# //' /etc/locale.gen \
    && sed -i '/en_US.UTF-8/s/^# //' /etc/locale.gen \
    && locale-gen
ENV LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8

# -----------------------------------------------------------------------------
#  Utilisateur non privilegie
# -----------------------------------------------------------------------------
#  On veut un utilisateur d'uid 1000 : c'est l'uid par defaut sur la plupart
#  des postes Linux, ce qui evite les problemes de droits sur les fichiers
#  montes depuis l'hote.
#
#  Depuis 24.04, l'image Ubuntu fournit un utilisateur `ubuntu` (uid 1000).
#  On ne S'APPUIE PAS sur cette hypothese : elle est vraie aujourd'hui, elle
#  n'est garantie par aucun contrat, et un `USER ubuntu` sur une image qui ne
#  le fournirait plus ferait echouer la construction avec un message obscur.
#  On cree donc l'utilisateur s'il manque. Le meme reflexe que partout
#  ailleurs dans ce depot : une hypothese verifiee vaut mieux qu'une
#  hypothese vraie.
RUN id -u ubuntu >/dev/null 2>&1 \
    || useradd --uid 1000 --user-group --create-home --shell /bin/bash ubuntu

USER ubuntu
WORKDIR /workspace

# Trace de l'environnement, affichee a chaque construction de l'image.
RUN echo "=== Chaine de production ===" \
    && g++ --version | head -1 \
    && clang++ --version | head -1 \
    && cmake --version | head -1 \
    && ninja --version \
    && python3 --version
