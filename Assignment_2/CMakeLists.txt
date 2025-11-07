cmake_minimum_required(VERSION 3.16)
project(Assignment_2_3D_kinetic_sculpture_animation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/common/include)

# Add executable
add_executable(Assignment_2_3D_kinetic_sculpture_animation main.cpp)

# Add shader files only (not OBJ files)
file(GLOB_RECURSE SHADER_FILES 
    "resources/vs/*"
    "resources/fs/*"
)

# Add shader files to the target
target_sources(Assignment_2_3D_kinetic_sculpture_animation PRIVATE ${SHADER_FILES})

# Organize files in IDE
source_group("Shaders" FILES ${SHADER_FILES})

# Link libraries
target_link_libraries(Assignment_2_3D_kinetic_sculpture_animation 
    common
    OpenGL::GL
    glfw
    glm::glm
)

# Copy resources (including shaders) to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION ${CMAKE_BINARY_DIR}/Assignment_2:3D_kinetic_sculpture_animation/resources/)
