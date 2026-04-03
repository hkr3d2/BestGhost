cmake_minimum_required(VERSION 3.15)

# Set the project name (must match your mod.json ID)
project(BestGhost)

# Find Geode
find_package(Geode REQUIRED)

# Create the mod
geode_mod()

# Add all your source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.hpp
)

# Add the sources to the mod
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

# Include directories (if needed)
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Set C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Link Geode (automatically handled by geode_mod() but explicit for clarity)
target_link_libraries(${PROJECT_NAME} PRIVATE Geode::Geode)
