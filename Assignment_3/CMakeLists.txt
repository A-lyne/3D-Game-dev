cmake_minimum_required(VERSION 3.16)
project(Assignment_3)

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
add_executable(Assignment_3 main.cpp)

# Add shader files
file(GLOB_RECURSE SHADER_FILES 
    "resources/vs/*"
    "resources/fs/*"
)

# Add shader files to the target
target_sources(Assignment_3 PRIVATE ${SHADER_FILES})

# Organize files in IDE
source_group("Shaders" FILES ${SHADER_FILES})

# Link libraries
target_link_libraries(Assignment_3 
    common
    OpenGL::GL
    glfw
    glm::glm
)

# Copy resources (including shaders and models) to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION ${CMAKE_BINARY_DIR}/Assignment_3/resources/)

# Set working directory for IDE (so resources can be found)
set_target_properties(Assignment_3 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/Assignment_3"
    XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/Assignment_3"
)

