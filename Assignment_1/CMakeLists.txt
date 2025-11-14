cmake_minimum_required(VERSION 3.16)
project(Assignment_1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)

# Add executable
add_executable(Assignment_1 
    main.cpp
)

# Include directories
target_include_directories(Assignment_1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/includes/learnopengl
    ${stb_SOURCE_DIR}
    ${glad_SOURCE_DIR}/include
    ${glm_SOURCE_DIR}
)

# Add shader files
file(GLOB_RECURSE SHADER_FILES 
    "*.vs"
    "*.fs"
)

# Add shader files to the target
target_sources(Assignment_1 PRIVATE ${SHADER_FILES})

# Organize files in IDE
source_group("Shaders" FILES ${SHADER_FILES})

# Link libraries
target_link_libraries(Assignment_1 
    common
    OpenGL::GL
    glfw
    glm::glm
)

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/transform.vs DESTINATION ${CMAKE_BINARY_DIR}/Assignment_1/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/transform.fs DESTINATION ${CMAKE_BINARY_DIR}/Assignment_1/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/transform.vs DESTINATION ${CMAKE_BINARY_DIR}/Assignment_1/Debug/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/transform.fs DESTINATION ${CMAKE_BINARY_DIR}/Assignment_1/Debug/)

# Copy resources to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION ${CMAKE_BINARY_DIR}/Assignment_1/resources/)

# Set working directory for IDE (so resources can be found)
set_target_properties(Assignment_1 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/Assignment_1"
    XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/Assignment_1"
)

# macOS specific
if(APPLE)
    target_link_libraries(Assignment_1 
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

