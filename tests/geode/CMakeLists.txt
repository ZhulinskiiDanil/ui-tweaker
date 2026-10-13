cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# The generated code compiled against the real Geode headers: a mod that's built, never installed or run.
#   cmake -S tests/geode -B build-geode -G Ninja -DREAL_INC=<the generator's third output> && cmake --build build-geode
# Geode.dll uses the release C runtime, as in the mod's own CMakeLists
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

project(ui-tweaker-codecheck VERSION 1.0.0)

# By default the generator's output next to this file (CI writes it there)
if (NOT REAL_INC)
    set(REAL_INC "${CMAKE_CURRENT_SOURCE_DIR}/real.inc")
endif()
if (NOT EXISTS "${REAL_INC}")
    message(FATAL_ERROR "REAL_INC: the generated code to compile (generate-samples' third output), ${REAL_INC} isn't there")
endif()
add_library(${PROJECT_NAME} SHARED main.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE REAL_INC="${REAL_INC}")

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
endif()
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME} DONT_INSTALL)
