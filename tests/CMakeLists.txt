include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2)
FetchContent_MakeAvailable(Catch2)

add_executable(VCOTunerTests
    CvScalingTests.cpp
    ClockCalibratorTests.cpp
    PeriodDetectorTests.cpp
    MeasurementStatisticsTests.cpp
    MeasurementErrorTests.cpp
    MeasurementTimingTests.cpp)

target_link_libraries(VCOTunerTests PRIVATE vcotuner_dsp Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(VCOTunerTests DISCOVERY_MODE PRE_TEST)

# Built and run separately from the Catch2 suite: it replaces global operator
# new to count allocations, which must not affect the other tests.
add_executable(VCOTunerRealtimeCheck RealtimeAllocationCheck.cpp)
target_link_libraries(VCOTunerRealtimeCheck PRIVATE vcotuner_dsp)
add_test(NAME realtime-allocation COMMAND VCOTunerRealtimeCheck)

# One target for everything ctest will try to run. CI builds this rather than
# naming each executable: a test registered with add_test() but never built
# reports "Not Run" and fails the suite, and the workflow should not have to be
# edited every time a target is added here.
add_custom_target(vcotuner_all_tests
    DEPENDS VCOTunerTests VCOTunerRealtimeCheck)
