cmake_minimum_required(VERSION 3.16)
project(Assignment_4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)

# Add executable
add_executable(Assignment_4 
    main.cpp
    stb_image_impl.cpp
)

# Include directories (using target_include_directories for better scoping)
target_include_directories(Assignment_4 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/includes/learnopengl
)

# Add shader files
file(GLOB_RECURSE SHADER_FILES 
    "*.vs"
    "*.fs"
)

# Add shader files to the target
target_sources(Assignment_4 PRIVATE ${SHADER_FILES})

# Organize files in IDE
source_group("Shaders" FILES ${SHADER_FILES})

# Link libraries
target_link_libraries(Assignment_4 
    common
    OpenGL::GL
    glfw
    glm::glm
    assimp::assimp
)

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/anim_model.vs DESTINATION ${CMAKE_BINARY_DIR}/Assignment_4/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/anim_model.fs DESTINATION ${CMAKE_BINARY_DIR}/Assignment_4/)

# Copy resources (including models and textures) to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION ${CMAKE_BINARY_DIR}/Assignment_4/resources/)

# Set working directory for IDE (so resources can be found)
set_target_properties(Assignment_4 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/Assignment_4"
    XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/Assignment_4"
)

