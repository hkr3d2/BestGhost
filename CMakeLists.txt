cmake_minimum_required(VERSION 3.20)

# This must match your mod.json ID
project(BestGhost)

# The official Geode action sets GEODE_SDK environment variable
# Use that to find Geode
if(DEFINED ENV{GEODE_SDK})
    set(GEODE_SDK $ENV{GEODE_SDK})
    list(APPEND CMAKE_PREFIX_PATH ${GEODE_SDK})
endif()

# Find Geode - this will work with the official action
find_package(Geode REQUIRED)

# Create the mod
geode_mod()

# Add ALL your source files
file(GLOB_RECURSE SOURCES 
    src/*.cpp
    src/*.hpp
)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Set C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Disable warnings as errors for now
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-error)
