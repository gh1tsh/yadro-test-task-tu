# This CMake file is for building unit tests target of the project


add_executable(tapedatainterface_unit_tests
                unit_tests.cpp
                ../TapeDev.cpp
                ../TapeSorter.cpp
                ../TapeDevConfig.cpp)

target_include_directories(tapedatainterface_unit_tests
                            PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(
    tapedatainterface_unit_tests
    PRIVATE gtest_main)
