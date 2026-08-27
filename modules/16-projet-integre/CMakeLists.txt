set(MODULE_ID 16-projet-integre)

training_library(${MODULE_ID}
    SOURCES
        src/fqms.cpp
)
# Dependances INTER-MODULES, documentees dans le SDD (section 1.1) :
#   mod04 : Mass, type fort de masse
#   mod07 : Result<T> et Status
#   mod10 : AlertMonitor, hysteresis et anti-rebond
# Ce graphe de dependances EST une donnee d'architecture : toute evolution de
# l'un de ces composants impose une analyse de changement (DO-178C 7.2.4).
target_link_libraries(mod_${MODULE_ID} PUBLIC
    mod_04-classes-invariants
    mod_07-erreurs-sans-exceptions
    mod_10-tests-bases-exigences
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

# Deux campagnes DISTINCTES, comme l'exige la table A-6 :
#   tests_16-projet-integre        -> exigences de BAS niveau  (A-6.3 / A-6.4)
#   tests_16-projet-integre-hlr    -> exigences de HAUT niveau (A-6.1 / A-6.2)
training_tests(${MODULE_ID}
    SOURCES
        tests/test_fqms_llr.cpp
)

add_executable(tests_${MODULE_ID}-hlr tests/test_fqms_hlr.cpp)
target_link_libraries(tests_${MODULE_ID}-hlr PRIVATE microtest mod_${MODULE_ID})
set_target_properties(tests_${MODULE_ID}-hlr PROPERTIES FOLDER "modules/${MODULE_ID}")
add_test(NAME ${MODULE_ID}-hlr COMMAND tests_${MODULE_ID}-hlr)
set_tests_properties(${MODULE_ID}-hlr PROPERTIES LABELS "module")
