set(MODULE_ID 02-pointeurs-references-const)

training_library(${MODULE_ID}
    SOURCES
        src/buffers.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_buffers.cpp
)
