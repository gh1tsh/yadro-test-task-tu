cmake_minimum_required(VERSION 3.2)

project(
    TapeDataInterface
    VERSION 0.1.0
    DESCRIPTION ""
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(tapedatainterface
                main.cpp
                TapeDev.cpp
                TapeDevConfig.cpp
                TapeSorter.cpp)
