set(MODULE_ID 12-couplage-donnees-controle)

training_library(${MODULE_ID}
    SOURCES
        src/chain.cpp
        src/coupling_trace.cpp
)
# Dependance inter-modules : la chaine reutilise Result<T> du module 07.
target_link_libraries(mod_${MODULE_ID} PUBLIC mod_07-erreurs-sans-exceptions)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_coupling.cpp
)
