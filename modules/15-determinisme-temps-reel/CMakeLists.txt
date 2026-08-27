set(MODULE_ID 15-determinisme-temps-reel)

training_library(${MODULE_ID}
    SOURCES
        src/fixed_point.cpp
        src/float_facts.cpp
        src/schedule.cpp
        src/hw_register.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_determinism.cpp
)
