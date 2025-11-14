cmake_minimum_required(VERSION 3.16)
project(3D_Game_Development)

# Set policy version to handle older dependencies
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Override GLAD's minimum CMake version requirement
set(CMAKE_MINIMUM_REQUIRED_VERSION 3.5)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Include FetchContent for downloading dependencies
include(FetchContent)

# Fetch GLFW
if(WIN32)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.9
    )
else()
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
    PATCH_COMMAND
        COMMAND sed -i "" "s/cmake_minimum_required(VERSION 3.1)/cmake_minimum_required(VERSION 3.5)/" CMakeLists.txt
)
endif()
FetchContent_MakeAvailable(glfw)
set(glfw3_DIR ${glfw_BINARY_DIR}/src)

# Fetch GLAD
if(WIN32)
    FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v0.1.36
    )
else()
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
    PATCH_COMMAND
        COMMAND sed -i "" "s/cmake_minimum_required(VERSION 3.0)/cmake_minimum_required(VERSION 3.5)/" CMakeLists.txt
        COMMAND sed -i "" "s/find_package(PythonInterp REQUIRED)/find_package(Python REQUIRED)/" CMakeLists.txt
)
endif()
FetchContent_MakeAvailable(glad)

# Fetch GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)
set(glm_DIR ${glm_BINARY_DIR})

# Fetch stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# Fetch Assimp (only if Assignment 4 is enabled)
# Note: Assimp build can take a while, be patient
if(BUILD_ASSIGNMENT_4)
    message(STATUS "Fetching Assimp library (this may take a while)...")
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.4.2
    )
    # Configure Assimp build options to minimize build time
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    # Only build what we need
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(assimp)
    set(assimp_DIR ${assimp_BINARY_DIR})
    message(STATUS "Assimp library ready")
endif()

# macOS specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

# Generate root_directory.h from template for learnopengl filesystem
# This is needed because filesystem.h uses #include "root_directory.h" (relative path)
configure_file(
    ${CMAKE_SOURCE_DIR}/configuration/root_directory.h.in
    ${CMAKE_SOURCE_DIR}/includes/learnopengl/root_directory.h
    @ONLY
)

# Add common library
add_subdirectory(common)

# Build options
option(BUILD_ASSIGNMENT_1 "Build Assignment 1" ON)
option(BUILD_ASSIGNMENT_3 "Build Assignment 3" ON)
option(BUILD_ASSIGNMENT_4 "Build Assignment 4" ON)

# Add Assignment 1: 2D Animation
if(BUILD_ASSIGNMENT_1)
add_subdirectory(Assignment_1)
endif()

# Add Assignment 3: Loading 3D Model, Camera Following, Object Intersection
if(BUILD_ASSIGNMENT_3)
add_subdirectory(Assignment_3)
endif()

# Add Assignment 4: 3D Model Animation
if(BUILD_ASSIGNMENT_4)
add_subdirectory(Assignment_4)
endif()