# -----------------------------------------------------------------------------
#  Briques communes a toute la formation.
#
#  avio_common : types, assertions, Span. Utilise par tous les modules.
#  microtest   : le harnais de test (contient le `main` des executables de test).
# -----------------------------------------------------------------------------

add_library(avio_common STATIC
    src/assert.cpp
)
target_include_directories(avio_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(avio_common PROPERTIES FOLDER "common")

add_library(microtest STATIC
    src/microtest_main.cpp
)
target_include_directories(microtest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(microtest PUBLIC avio_common)
# DEVIATION LOCALISEE : std::fopen declenche C4996 sous MSVC, qui pousse vers
# fopen_s (Annexe K du C11, extension non portable et non implementee par GCC
# ni par la plupart des compilateurs embarques). On privilegie la portabilite
# et on limite la derogation a cette seule cible, plutot que de l'appliquer a
# tout le projet. C'est exactement la demarche attendue pour une deviation a
# un standard de codage : locale, justifiee, tracee.
target_compile_definitions(microtest PRIVATE _CRT_SECURE_NO_WARNINGS)
set_target_properties(microtest PROPERTIES FOLDER "common")

# Auto-test du harnais lui-meme. En DO-330, un outil de verification doit lui
# aussi faire l'objet d'une verification : ce sont les "tests de l'outil".
add_executable(tests_microtest tests/test_microtest.cpp)
target_link_libraries(tests_microtest PRIVATE microtest)
set_target_properties(tests_microtest PROPERTIES FOLDER "common")
add_test(NAME microtest_selftest COMMAND tests_microtest)
set_tests_properties(microtest_selftest PROPERTIES LABELS "outil")
