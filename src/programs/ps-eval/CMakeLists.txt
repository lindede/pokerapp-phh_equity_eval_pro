add_executable(ps-eval main.cpp)
target_link_libraries(ps-eval PRIVATE pokerstove::penum pokerstove::peval pokerstove::util)
