set(MODULE_ID 13-standards-codage)

training_library(${MODULE_ID}
    SOURCES
        src/bcd.cpp
)
target_link_libraries(mod_${MODULE_ID} PUBLIC mod_07-erreurs-sans-exceptions)

# -----------------------------------------------------------------------------
#  Cible ISOLEE pour le code NON CONFORME.
#
#  clang-tidy est desactive et les avertissements sont relaches, UNIQUEMENT sur
#  cette cible. C'est le traitement que l'on reserve en projet reel au code
#  HERITE ou TIERS que l'on ne peut pas modifier :
#    * on l'isole dans sa propre unite de compilation et son propre espace de
#      noms ;
#    * on documente la derogation a l'endroit exact ou elle s'applique ;
#    * on ne laisse pas ses avertissements noyer ceux du code neuf.
#
#  Le contre-exemple a ne jamais faire : desactiver /W4 ou clang-tidy pour tout
#  le projet parce qu'un fichier est bruyant.
# -----------------------------------------------------------------------------
add_library(mod_13_nonconforming STATIC
    src/nonconforming.cpp
)
target_link_libraries(mod_13_nonconforming PUBLIC mod_${MODULE_ID})
set_target_properties(mod_13_nonconforming PROPERTIES
    CXX_CLANG_TIDY ""
    FOLDER "modules/${MODULE_ID}")

# NOTE : MSVC emet deux messages D9025 ("overriding /W4 with /W1" et
# "overriding /WX with /WX-") lors de la compilation de cette cible. Ils sont
# ATTENDUS : ils sont la trace, dans le journal de build, que la derogation a
# bien lieu et ou elle a lieu. On ne les supprime pas -- une derogation
# silencieuse serait pire qu'une derogation bruyante.
if(MSVC)
    target_compile_options(mod_13_nonconforming PRIVATE /W1 /WX-)
else()
    target_compile_options(mod_13_nonconforming PRIVATE -w)
endif()

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)
target_link_libraries(demo_${MODULE_ID} PRIVATE mod_13_nonconforming)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_bcd.cpp
)
target_link_libraries(tests_${MODULE_ID} PRIVATE mod_13_nonconforming)
