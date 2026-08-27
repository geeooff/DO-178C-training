set(MODULE_ID 09-exigences-tracabilite)

training_library(${MODULE_ID}
    SOURCES
        src/altitude.cpp
)
# Dependance INTER-MODULES explicite : ADC-ALT reutilise Result<T> du module 07.
# En DO-178C, ce lien est une donnee d'architecture (couplage de donnees) :
# il doit apparaitre dans le SDD et etre analyse (module 12).
target_link_libraries(mod_${MODULE_ID} PUBLIC mod_07-erreurs-sans-exceptions)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_altitude.cpp
)
