set(MODULE_ID 10-tests-bases-exigences)

training_library(${MODULE_ID}
    SOURCES
        src/alert_monitor.cpp
)

training_demo(${MODULE_ID}
    SOURCES
        src/main.cpp
)

training_tests(${MODULE_ID}
    SOURCES
        tests/test_alert_monitor.cpp
)
