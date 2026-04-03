cmake_minimum_required(VERSION 3.20)

project(BestGhost)

# Manually add Geode's cmake directory
if(DEFINED ENV{GEODE_SDK})
    set(GEODE_CMAKE "$ENV{GEODE_SDK}/cmake")
else()
    set(GEODE_CMAKE "C:/geode-sdk/cmake")
endif()

include(${GEODE_CMAKE}/GeodeMacros.cmake)

geode_mod()

file(GLOB_RECURSE SOURCES src/*.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Add Geode includes manually
target_include_directories(${PROJECT_NAME} PRIVATE
    ${GEODE_CMAKE}/../include
)
