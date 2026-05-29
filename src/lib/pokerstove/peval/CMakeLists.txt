find_package(Threads)

file(GLOB test_sources *.test.cpp)
file(GLOB lib_sources *.cpp)
foreach(source_file ${test_sources})
    list(REMOVE_ITEM lib_sources ${source_file})
endforeach()

add_library(peval ${lib_sources})
add_library(pokerstove::peval ALIAS peval)

target_include_directories(peval
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(peval PUBLIC pokerstove::util Threads::Threads)

if(BUILD_TESTING)
    add_executable(peval_tests ${test_sources})
    target_link_libraries(peval_tests PRIVATE peval gtest gtest_main Threads::Threads)
    add_test(NAME TestPeval COMMAND peval_tests)
endif()
