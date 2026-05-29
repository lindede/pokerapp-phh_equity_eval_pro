cmake_minimum_required(VERSION 3.20)

project(
    pokerapp-phh_equity_eval
    VERSION 0.1.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Build unit tests" ON)
option(BUILD_PYTHON "Build Python bindings" OFF)
option(BUILD_WHEEL "Build Python wheel" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

if(BUILD_TESTING AND NOT BUILD_WHEEL)
    enable_testing()
    add_subdirectory(src/ext/googletest)
endif()

add_subdirectory(src/lib)

if(BUILD_PYTHON OR BUILD_WHEEL)
    add_subdirectory(src/lib/python)
endif()

if(NOT BUILD_WHEEL)
    add_subdirectory(src/programs)
    install(TARGETS peval penum ps-colex ps-eval ps-lut)
endif()
