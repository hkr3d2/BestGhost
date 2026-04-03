cmake_minimum_required(VERSION 3.20)
project(BestGhost)

find_package(Geode REQUIRED)
geode_mod()

# Add all cpp files
file(GLOB_RECURSE SOURCES src/*.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
)
