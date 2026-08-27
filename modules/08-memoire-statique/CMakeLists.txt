set(MODULE_ID 08-memoire-statique)

training_library(${MODULE_ID}
    SOURCES
        src/stack_analysis.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_memory.cpp
)
