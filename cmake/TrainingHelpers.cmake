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
function(training_setup_compiler_flags)
    if(MSVC)
        # /W4        : niveau d'avertissement eleve
        # /permissive-: conformite stricte au standard (desactive les extensions MS)
        # /Zc:__cplusplus : rend la macro __cplusplus correcte (sinon MSVC ment)
        # /utf-8     : sources ET executables en UTF-8
        add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8 /EHsc)
        if(TRAINING_WARNINGS_AS_ERRORS)
            add_compile_options(/WX)
        endif()
        # Determinisme des flottants : voir module 15.
        # /fp:precise est le defaut MSVC ; on l'exprime explicitement pour que
        # ce choix soit TRACABLE et non implicite.
        add_compile_options(/fp:precise)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(TRAINING_WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    endif()

    if(TRAINING_ENABLE_CLANG_TIDY)
        find_program(CLANG_TIDY_EXE NAMES clang-tidy)
        if(CLANG_TIDY_EXE)
            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}" PARENT_SCOPE)
            message(STATUS "clang-tidy trouve : ${CLANG_TIDY_EXE}")
        else()
            message(WARNING "clang-tidy demande mais introuvable dans le PATH.")
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
