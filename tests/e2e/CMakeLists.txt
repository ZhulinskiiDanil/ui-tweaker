cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# The end-to-end check: "Copy all" from an edit in the game (E2E_CODE) built into a mod as a mod developer pastes it,
# with a check of where the nodes end up (E2E_EXPECTED). tools/e2e.sh builds it, puts it in the game for one run and
# takes it out again
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

project(ui-tweaker-e2e VERSION 1.0.0)

if (NOT E2E_CODE OR NOT E2E_EXPECTED)
    message(FATAL_ERROR "E2E_CODE and E2E_EXPECTED: the exported code and where its nodes are to end up")
endif()
add_library(${PROJECT_NAME} SHARED main.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE E2E_CODE="${E2E_CODE}" E2E_EXPECTED="${E2E_EXPECTED}")

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
endif()
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME} DONT_INSTALL)
