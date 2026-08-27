# =============================================================================
#  Fonctions utilitaires du projet de formation.
#
#  Chaque module suit la meme convention, ce qui rend la structure du depot
#  previsible : c'est une exigence implicite de la DO-178C (le SDP -- Software
#  Development Plan -- decrit les standards de structure du code).
#
#      modules/<id>/include/   en-tetes publics du module
#      modules/<id>/src/       implementation
#      modules/<id>/tests/     tests unitaires (bases sur les exigences)
#      modules/<id>/exercices/ squelettes a completer + solutions
# =============================================================================

# -----------------------------------------------------------------------------
# training_setup_compiler_flags()
#   Applique globalement les options de compilation "rigoureuses".
# -----------------------------------------------------------------------------
#  PORTABILITE
#  -----------
#  Ce projet se compile a l'identique avec MSVC, GCC et Clang, sur Windows,
#  Linux et macOS. Ce n'est pas de la coquetterie : c'est une propriete que la
#  DO-178C rend precieuse.
#
#    * Chaque chaine d'outils detecte des defauts que les autres laissent
#      passer. MSVC /W4 ne signale pas ce que GCC -Wconversion voit, et
#      inversement. Compiler avec trois compilateurs, c'est trois analyses
#      statiques pour le prix d'une.
#    * Un code qui ne compile que sur une chaine est un code dont on ne sait
#      pas quelles hypotheses implicites il porte.
#    * Le jour ou le compilateur cible change -- et il change, sur un
#      programme qui vit trente ans -- le portage est deja fait.
#
#  ATTENTION : compiler sur trois chaines ne dispense de RIEN. Le SECI
#  (module 00) fige UNE chaine, UNE version, UN jeu d'options. La portabilite
#  est un outil de qualite pendant le developpement, pas une propriete du
#  produit certifie.

function(training_setup_compiler_flags)
    if(MSVC)
        # /W4             : niveau d'avertissement eleve
        # /permissive-    : conformite stricte au standard (pas d'extensions MS)
        # /Zc:__cplusplus : rend la macro __cplusplus correcte (sinon MSVC ment)
        # /utf-8          : sources ET executables en UTF-8
        add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8 /EHsc)
        if(TRAINING_WARNINGS_AS_ERRORS)
            add_compile_options(/WX)
        endif()
        # Determinisme des flottants : voir module 15.
        # /fp:precise est le defaut MSVC ; on l'exprime explicitement pour que
        # ce choix soit TRACABLE et non implicite.
        add_compile_options(/fp:precise)
    else()
        # Equivalents GCC / Clang du jeu d'options ci-dessus.
        #
        #   -Wall -Wextra   : socle commun
        #   -Wpedantic      : refuse les extensions du compilateur
        #   -Wconversion    : conversions implicites qui perdent de
        #                     l'information -- LA lecon du module 01, et ce que
        #                     MSVC ne detecte PAS
        #   -Wshadow        : masquage de variable, source classique de bugs
        #   -Wold-style-cast: impose static_cast (regle R-06 du module 13)
        #   -Wdouble-promotion : promotion silencieuse float -> double
        #                     (module 15 : elle change le resultat)
        #   -Wformat=2      : verification stricte des chaines de format
        add_compile_options(
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Wold-style-cast
            -Wdouble-promotion
            -Wformat=2)

        if(TRAINING_WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()

        # Determinisme des flottants, equivalent de /fp:precise.
        # -ffp-contract=off interdit la fusion multiplication-addition (FMA) :
        # sans elle, `a*b+c` peut etre calcule en une seule instruction, sans
        # arrondi intermediaire, et donner un resultat DIFFERENT de celui de
        # MSVC. Voir module 15.
        add_compile_options(-fno-fast-math -ffp-contract=off)
    endif()

    if(TRAINING_ENABLE_CLANG_TIDY)
        # Sur Windows, clang-tidy est livre avec Visual Studio mais n'est pas
        # dans le PATH par defaut : on va le chercher la ou il se trouve.
        find_program(CLANG_TIDY_EXE
            NAMES clang-tidy
            HINTS "$ENV{VCToolsInstallDir}/../../Llvm/x64/bin"
                  "$ENV{VSINSTALLDIR}/VC/Tools/Llvm/x64/bin")
        if(CLANG_TIDY_EXE)
            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}" PARENT_SCOPE)
            message(STATUS "clang-tidy trouve : ${CLANG_TIDY_EXE}")
        else()
            message(WARNING "clang-tidy demande mais introuvable dans le PATH.")
        endif()
    endif()

    if(TRAINING_ENABLE_COVERAGE)
        if(MSVC)
            message(WARNING
                "TRAINING_ENABLE_COVERAGE est sans effet avec MSVC. "
                "Sous Windows, utilisez scripts/coverage.ps1 (OpenCppCoverage), "
                "qui instrumente le binaire a l'execution plutot qu'a la compilation.")
        else()
            # --coverage active l'instrumentation gcov (GCC) ou son equivalent
            # (Clang). Pas d'optimisation : le code genere doit rester tracable
            # au code source, sans quoi les compteurs de lignes n'ont plus de
            # sens -- c'est exactement la problematique de l'objectif A-7.9
            # (couverture du code objet) vue par le petit bout de la lorgnette.
            message(STATUS "Couverture : instrumentation gcov activee")
            add_compile_options(--coverage -O0 -g)
            add_link_options(--coverage)
        endif()
    endif()

    if(TRAINING_ENABLE_SANITIZERS)
        if(MSVC)
            # MSVC ne fournit que l'AddressSanitizer, et il est incompatible
            # avec /RTC1 (verifications d'execution du mode Debug).
            message(STATUS "Sanitizers : AddressSanitizer (MSVC)")
            add_compile_options(/fsanitize=address)
        else()
            # ASan   : debordements, usage apres liberation, fuites
            # UBSan  : comportements INDEFINIS -- debordement signe, decalage
            #          invalide, dereferencement nul, conversion hors domaine.
            #          C'est l'outil qui rend TANGIBLE tout le module 01.
            message(STATUS "Sanitizers : Address + UndefinedBehavior")
            add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer
                                -fno-sanitize-recover=all)
            add_link_options(-fsanitize=address,undefined)
        endif()
    endif()
endfunction()

# -----------------------------------------------------------------------------
# training_library(<id> SOURCES <fichiers...>)
#   Cree la bibliotheque statique du module : mod_<id>
#   Le repertoire include/ du module devient PUBLIC (visible des consommateurs).
# -----------------------------------------------------------------------------
function(training_library id)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})
    set(target "mod_${id}")
    add_library(${target} STATIC ${ARG_SOURCES})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    endif()
    target_link_libraries(${target} PUBLIC avio_common)
    set_target_properties(${target} PROPERTIES FOLDER "modules/${id}")
endfunction()

# -----------------------------------------------------------------------------
# training_demo(<id> SOURCES <fichiers...>)
#   Cree l'executable de demonstration : demo_<id>
#   C'est le programme que l'on lance pour "voir" le cours du jour s'executer.
# -----------------------------------------------------------------------------
function(training_demo id)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})
    set(target "demo_${id}")
    add_executable(${target} ${ARG_SOURCES})
    if(TARGET mod_${id})
        target_link_libraries(${target} PRIVATE mod_${id})
    else()
        target_link_libraries(${target} PRIVATE avio_common)
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include)
            target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        endif()
    endif()
    set_target_properties(${target} PROPERTIES FOLDER "modules/${id}")
endfunction()

# -----------------------------------------------------------------------------
# training_tests(<id> SOURCES <fichiers...>)
#   Cree l'executable de test : tests_<id>, et l'enregistre aupres de CTest.
#   En DO-178C, l'execution des tests doit etre REPETABLE et son resultat
#   enregistre : CTest fournit ce journal (Testing/Temporary/LastTest.log).
# -----------------------------------------------------------------------------
function(training_tests id)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})
    set(target "tests_${id}")
    add_executable(${target} ${ARG_SOURCES})
    target_link_libraries(${target} PRIVATE microtest)
    if(TARGET mod_${id})
        target_link_libraries(${target} PRIVATE mod_${id})
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    endif()
    set_target_properties(${target} PROPERTIES FOLDER "modules/${id}")
    add_test(NAME ${id} COMMAND ${target})
    set_tests_properties(${id} PROPERTIES LABELS "module")
endfunction()

# -----------------------------------------------------------------------------
# training_exercise(<id> <nom> SOURCES <fichiers...>)
#   Cible optionnelle (option TRAINING_BUILD_EXERCISES) : les squelettes
#   d'exercices ne compilent pas tant qu'ils ne sont pas completes, il ne faut
#   donc pas casser le build par defaut.
# -----------------------------------------------------------------------------
function(training_exercise id name)
    if(NOT TRAINING_BUILD_EXERCISES)
        return()
    endif()
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})
    set(target "exo_${id}_${name}")
    add_executable(${target} ${ARG_SOURCES})
    target_link_libraries(${target} PRIVATE avio_common)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    endif()
    set_target_properties(${target} PROPERTIES FOLDER "modules/${id}/exercices")
endfunction()

# -----------------------------------------------------------------------------
# training_solution(<id> <nom> SOURCES <fichiers...>)
#   Les solutions, elles, compilent toujours : elles font partie du perimetre
#   verifie par la CI.
# -----------------------------------------------------------------------------
function(training_solution id name)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})
    set(target "sol_${id}_${name}")
    add_executable(${target} ${ARG_SOURCES})
    target_link_libraries(${target} PRIVATE avio_common microtest)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    endif()
    set_target_properties(${target} PROPERTIES FOLDER "modules/${id}/solutions")
    add_test(NAME ${id}_solution_${name} COMMAND ${target})
    set_tests_properties(${id}_solution_${name} PROPERTIES LABELS "solution")
endfunction()
