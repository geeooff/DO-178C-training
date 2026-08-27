set(MODULE_ID 07-erreurs-sans-exceptions)

training_library(${MODULE_ID}
    SOURCES
        src/result.cpp
        src/arinc429.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_result.cpp
)
