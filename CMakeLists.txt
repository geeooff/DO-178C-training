# =============================================================================
#  DO-178C-training : formation C++ / DO-178C
#
#  Ce fichier est le point d'entree du "build system". En C#, MSBuild deduit
#  presque tout de la structure du projet (.csproj). En C++, RIEN n'est
#  implicite : chaque fichier source, chaque chemin d'include, chaque option de
#  compilation doit etre declare. C'est verbeux, mais c'est exactement ce que
#  la DO-178C demande : un environnement de production du code source
#  entierement DECRIT et REPRODUCTIBLE (objectifs du processus de gestion de
#  configuration, DO-178C section 7).
# =============================================================================
cmake_minimum_required(VERSION 3.21)

project(DO178C_Training
    VERSION 1.0.0
    DESCRIPTION "Formation C++ et DO-178C pour developpeur C#"
    LANGUAGES CXX)

# --- Standard du langage ------------------------------------------------------
# C++17 est le choix realiste pour l'avionique en 2020+ :
#   * MISRA C++:2023 cible C++17 ;
#   * AUTOSAR C++14 (fusionne dans MISRA C++:2023) ciblait C++14 ;
#   * C++20/23 arrivent seulement dans les nouveaux programmes.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)   # pas d'extensions proprietaires : portabilite

# --- Options de la formation --------------------------------------------------
option(TRAINING_WARNINGS_AS_ERRORS "Traiter les avertissements comme des erreurs" OFF)
option(TRAINING_BUILD_EXERCISES    "Compiler les squelettes d'exercices"          OFF)
option(TRAINING_ENABLE_CLANG_TIDY  "Activer l'analyse statique clang-tidy"        OFF)
option(TRAINING_ENABLE_SANITIZERS  "Activer AddressSanitizer et UBSanitizer"      OFF)
option(TRAINING_ENABLE_COVERAGE    "Instrumenter pour gcov / llvm-cov"            OFF)

# Sorties regroupees : plus simple a retrouver, et un seul repertoire a archiver
# dans la baseline de configuration.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Genere compile_commands.json (consomme par clang-tidy / clangd).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(TrainingHelpers)

training_setup_compiler_flags()

enable_testing()

add_subdirectory(common)
add_subdirectory(modules)

# Regroupement dans l'explorateur de solutions de Visual Studio.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Ce resume est le point de depart du SECI (module 00 et module 14) : il
# affiche, a chaque configuration, l'environnement exact de production.
message(STATUS "")
message(STATUS "=== DO-178C-training ===")
message(STATUS "  Systeme hote       : ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS "  Compilateur        : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Chemin compilateur : ${CMAKE_CXX_COMPILER}")
message(STATUS "  Standard C++       : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Type de build      : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Warnings = erreurs : ${TRAINING_WARNINGS_AS_ERRORS}")
message(STATUS "  clang-tidy         : ${TRAINING_ENABLE_CLANG_TIDY}")
message(STATUS "  Sanitizers         : ${TRAINING_ENABLE_SANITIZERS}")
message(STATUS "  Couverture         : ${TRAINING_ENABLE_COVERAGE}")
message(STATUS "  Exercices compiles : ${TRAINING_BUILD_EXERCISES}")
message(STATUS "")
