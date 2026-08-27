set(MODULE_ID 01-types-et-memoire)

training_library(${MODULE_ID}
    SOURCES
        src/safe_arith.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_safe_arith.cpp
)
