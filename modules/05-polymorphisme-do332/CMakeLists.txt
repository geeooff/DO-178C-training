set(MODULE_ID 05-polymorphisme-do332)

training_library(${MODULE_ID}
    SOURCES
        src/sensors.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_sensors.cpp
)
