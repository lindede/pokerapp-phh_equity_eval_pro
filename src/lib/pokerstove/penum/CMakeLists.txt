find_package(Threads)

file(GLOB test_sources *.test.cpp)
file(GLOB lib_sources *.cpp)
foreach(source_file ${test_sources})
    list(REMOVE_ITEM lib_sources ${source_file})
endforeach()

add_library(penum ${lib_sources})
add_library(pokerstove::penum ALIAS penum)

target_include_directories(penum
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(penum PUBLIC pokerstove::peval Threads::Threads)

if(BUILD_TESTING)
    add_executable(penum_tests ${test_sources})
    target_link_libraries(penum_tests PRIVATE penum gtest gtest_main Threads::Threads)
    add_test(NAME TestPenum COMMAND penum_tests)
endif()
