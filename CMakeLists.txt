cmake_minimum_required(VERSION 3.20)

project(BestGhost)

# The Geode action clones the SDK to this folder
set(GEODE_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/geode-sdk-clone")

if(EXISTS "${GEODE_SDK_PATH}/cmake/GeodeMacros.cmake")
    include("${GEODE_SDK_PATH}/cmake/GeodeMacros.cmake")
else()
    message(FATAL_ERROR "Geode SDK not found at ${GEODE_SDK_PATH}")
endif()

# Collect all source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# Create the mod
add_geode_mod(BestGhost ${SOURCES})

# C++ standard
set_target_properties(BestGhost PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
