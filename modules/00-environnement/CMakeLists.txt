set(MODULE_ID 00-environnement)

training_library(${MODULE_ID}
    SOURCES
        src/build_info.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_build_info.cpp
)