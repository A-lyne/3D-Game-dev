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
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
    PATCH_COMMAND
        COMMAND sed -i "" "s/cmake_minimum_required(VERSION 3.1)/cmake_minimum_required(VERSION 3.5)/" CMakeLists.txt
)
FetchContent_MakeAvailable(glfw)

# Fetch GLAD
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
    PATCH_COMMAND
        COMMAND sed -i "" "s/cmake_minimum_required(VERSION 3.0)/cmake_minimum_required(VERSION 3.5)/" CMakeLists.txt
        COMMAND sed -i "" "s/find_package(PythonInterp REQUIRED)/find_package(Python REQUIRED)/" CMakeLists.txt
)
FetchContent_MakeAvailable(glad)

# Fetch GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# Fetch stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# macOS specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

# Add common library
add_subdirectory(common)

# Add Assignment 2: 3D Kinetic Sculpture Animation
add_subdirectory("Assignment_2:3D_kinetic_sculpture_animation")

# Add Assignment 3: Loading 3D Model, Camera Following, Object Intersection
add_subdirectory(Assignment_3)

# Add Final Project
add_subdirectory(Final_Project)