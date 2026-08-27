set(MODULE_ID 03-raii-cycle-de-vie)

training_library(${MODULE_ID}
    SOURCES
        src/lifetime.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_lifetime.cpp
)
