add_library(util INTERFACE)
add_library(pokerstove::util ALIAS util)

target_include_directories(util INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(BUILD_TESTING)
    file(GLOB test_sources *.test.cpp)
    add_executable(util_tests ${test_sources})
    target_link_libraries(util_tests PRIVATE util gtest gtest_main)
    add_test(NAME TestUtil COMMAND util_tests)
endif()
