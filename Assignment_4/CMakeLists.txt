cmake_minimum_required(VERSION 3.16)
project(Assignment_4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)

# Use assimp from FetchContent if available, otherwise try to find it
if(DEFINED assimp_SOURCE_DIR AND EXISTS ${assimp_SOURCE_DIR})
    # Assimp is available from FetchContent
    message(STATUS "Using Assimp from FetchContent")
    # Assimp target should be available after FetchContent_MakeAvailable
    if(NOT TARGET assimp::assimp)
        # Fallback: try to find the target with different name
        find_package(assimp REQUIRED)
    endif()
else()
    # Try to find system-installed Assimp
    find_package(assimp REQUIRED)
endif()

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

# Add assimp include directory if using FetchContent
if(DEFINED assimp_SOURCE_DIR AND EXISTS ${assimp_SOURCE_DIR})
    target_include_directories(Assignment_4 PRIVATE ${assimp_SOURCE_DIR}/include)
endif()

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

# Copy shaders to build directory (both root and Debug for compatibility)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/anim_model.vs DESTINATION ${CMAKE_BINARY_DIR}/Assignment_4/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/anim_model.fs DESTINATION ${CMAKE_BINARY_DIR}/Assignment_4/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/anim_model.vs DESTINATION ${CMAKE_BINARY_DIR}/Assignment_4/Debug/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/anim_model.fs DESTINATION ${CMAKE_BINARY_DIR}/Assignment_4/Debug/)

# Copy resources (including models and textures) to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION ${CMAKE_BINARY_DIR}/Assignment_4/resources/)

# Set working directory for IDE (so resources can be found)
set_target_properties(Assignment_4 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/Assignment_4"
    XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/Assignment_4"
)

