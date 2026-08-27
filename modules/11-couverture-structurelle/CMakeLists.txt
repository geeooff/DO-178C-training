set(MODULE_ID 11-couverture-structurelle)

training_library(${MODULE_ID}
    SOURCES
        src/gpws.cpp
        src/mcdc.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_gpws.cpp
)
