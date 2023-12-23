cmake_minimum_required(VERSION 3.22)

set(CPM_DOWNLOAD_VERSION 0.35.5)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

project(kai)

CPMAddPackage("gh:fmtlib/fmt#9.1.0")
CPMAddPackage("gh:SRombauts/SQLiteCpp#3.3.0")
CPMAddPackage("gh:nlohmann/json#v3.11.2")
CPMAddPackage("gh:gabime/spdlog#v1.12.0")
CPMAddPackage("gh:robinlinden/libsodium-cmake#master")
CPMAddPackage("gh:effolkronium/random#v1.5.0")
CPMAddPackage("gh:luau-lang/luau#0.607")
CPMAddPackage("gh:kunitoki/LuaBridge3#3.0-rc4")

SET(LUABRIDGE_TESTING OFF)
SET(LUAU_BUILD_CLI OFF)
SET(LUAU_BUILD_TESTS OFF)
SET(LUAU_BUILD_BENCHMARKS OFF)

find_package(SQLite3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system program_options regex)

set(KAI_INCLUDE_DIRS
        include
        ${Boost_INCLUDE_DIRS}
        ${fmt_SOURCE_DIR}/include
        ${eff_random_SOURCE_DIR}/include
        ${SQLiteCpp_SOURCE_DIR}/include
        ${nlohmann_json_SOURCE_DIR}/single_include
        ${spdlog_SOURCE_DIR}/include
        ${sodium_SOURCE_DIR}/libsodium/src/libsodium/include
        ${effolkronium_random_SOURCE_DIR}/include
        ${Luau_SOURCE_DIR}/VM/include
        ${Luau_SOURCE_DIR}/Compiler/include
        ${LuaBridge3_SOURCE_DIR}/Source
        )

file(GLOB_RECURSE KAI_INCLUDE include/*.h)
file(GLOB_RECURSE KAI_SRC src/*.cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive -w")

set(BUILD_TESTING OFF)

set(KAI_LINK_LIBRARIES
        fmt::fmt ${SQLite3_LIBRARIES} ${Boost_LIBRARIES}
        SQLiteCpp sqlite3 bsd
        Luau.VM Luau.Common Luau.Compiler
        sodium
        effolkronium_random
        )

# this is the core library we're making.
add_library(kaimud ${KAI_INCLUDE} ${KAI_SRC})
# everything we make will link to it.
target_link_libraries(kaimud ${KAI_LINK_LIBRARIES})
target_include_directories(kaimud PUBLIC ${KAI_INCLUDE_DIRS})

# the various binaries
add_executable(kai apps/kai.cpp)
target_link_libraries(kai kaimud ${KAI_LINK_LIBRARIES})
target_include_directories(kai PUBLIC ${KAI_INCLUDE_DIRS})


SET(kai_link ${CMAKE_INSTALL_PREFIX}/bin/)