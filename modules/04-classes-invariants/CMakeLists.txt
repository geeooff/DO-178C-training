set(MODULE_ID 04-classes-invariants)

training_library(${MODULE_ID}
    SOURCES
        src/units.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_units.cpp
)
