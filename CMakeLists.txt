cmake_minimum_required(VERSION 3.12)
project(net_on_chip)


set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SYSTEM_NAME LINUX)
set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++)
message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
set(CMAKE_BUILD_TYPE Debug)

set(MAIN_CPP "test_src/test_routerchannel.cpp")
file(GLOB SOURCE_CPP "src/*.cpp")
# file(GLOB TEST_CPP "test/*.cpp")
set(PROJECT_INCLUDE_DIR "src/" "test/" "include/")

set(SYSTEMC_INCLUDE_DIR "/usr/local/include")
set(SYSTEMC_LIBRARY_DIR "/usr/local/lib")
include_directories(${SYSTEMC_INCLUDE_DIR})
link_directories(${SYSTEMC_LIBRARY_DIR})

# 检查编译器版本
execute_process(
    COMMAND ${CMAKE_C_COMPILER} --version
    OUTPUT_VARIABLE GCC_VERSION
    ERROR_QUIET
)
execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} --version
    OUTPUT_VARIABLE GPP_VERSION
    ERROR_QUIET
)

include_directories(${PROJECT_INCLUDE_DIR})
include_directories(${SYSTEMC_INCLUDE_DIR})
link_directories(${SYSTEMC_LIBRARY_DIR})

add_executable( NOC ${SOURCE_CPP} ${MAIN_CPP})
target_link_libraries(NOC systemc)
