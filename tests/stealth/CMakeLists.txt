# Standalone unit tests for the StealthAmnezia additions, built against the client sources:
#   cmake -S tests/stealth -B build-tests && cmake --build build-tests && ctest --test-dir build-tests
cmake_minimum_required(VERSION 3.16)
project(stealth_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../client)
file(GLOB PROTOCOL_CONFIG_SOURCES ${CLIENT_DIR}/core/models/protocols/*.cpp)

add_library(stealth_core STATIC
    ${CLIENT_DIR}/core/utils/selfhosted/serverStatePatcher.cpp
    ${CLIENT_DIR}/core/models/containerConfig.cpp
    ${CLIENT_DIR}/core/models/protocolConfig.cpp
    ${CLIENT_DIR}/core/utils/containers/containerUtils.cpp
    ${CLIENT_DIR}/core/protocols/protocolUtils.cpp
    ${CLIENT_DIR}/core/utils/containerEnum.h
    ${CLIENT_DIR}/core/utils/protocolEnum.h
    ${PROTOCOL_CONFIG_SOURCES}
)
target_include_directories(stealth_core PUBLIC ${CLIENT_DIR} ${CLIENT_DIR}/..)
target_link_libraries(stealth_core PUBLIC Qt6::Core)

enable_testing()
foreach(test tst_serverStatePatcher)
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} stealth_core Qt6::Test)
    add_test(NAME ${test} COMMAND ${test})
endforeach()
