set(MODULE_ID 14-configuration-qualite)

training_library(${MODULE_ID}
    SOURCES
        src/identity.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_identity.cpp
)
