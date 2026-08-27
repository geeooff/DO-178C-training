set(MODULE_ID 06-templates-constexpr)

training_library(${MODULE_ID}
    SOURCES
        src/compile_time.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_templates.cpp
)
