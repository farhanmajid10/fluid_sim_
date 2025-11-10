cmake_minimum_required(VERSION 3.10)
project(IncrementalLBM_Step2)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "/W4")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Add FLGL as a subdirectory
add_subdirectory(lib/flgl)

# Create executable
add_executable(${PROJECT_NAME} 
    src/main.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/lib/flgl/inc
    ${CMAKE_SOURCE_DIR}/lib/flgl/lib/glad/include
    ${CMAKE_SOURCE_DIR}/lib/flgl/lib/glfw/include
    ${CMAKE_SOURCE_DIR}/lib/flgl/lib/glm
    ${CMAKE_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIRS}
)   

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    FLGL
    ${OPENGL_LIBRARIES}
)

# Platform-specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
elseif(UNIX)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ${X11_LIBRARIES}
        ${CMAKE_DL_LIBS}
        Threads::Threads
    )
endif()

# Create shader directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib/flgl/res/default_shaders)

# Step 2: Add advection and display shaders
set(SHADER_NAMES
    display
    advection
)

# Copy shaders to all possible locations FLGL might check
foreach(SHADER_NAME ${SHADER_NAMES})
    # Copy to build/shaders (our intended location)
    configure_file(
        ${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME}.vert
        ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.vert
        COPYONLY
    )
    configure_file(
        ${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME}.frag
        ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.frag
        COPYONLY
    )
    
    # Copy to where FLGL expects them with .glsl extension
    configure_file(
        ${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME}.vert
        ${CMAKE_BINARY_DIR}/lib/flgl/res/default_shaders/${SHADER_NAME}.glsl
        COPYONLY
    )
    configure_file(
        ${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME}.frag
        ${CMAKE_BINARY_DIR}/lib/flgl/res/default_shaders/${SHADER_NAME}_frag.glsl
        COPYONLY
    )
endforeach()

# Add custom target for shader copying
add_custom_target(copy_shaders ALL
    COMMENT "Copying Step 2 shaders (display + advection)"
)

# Make executable depend on shader copying
add_dependencies(${PROJECT_NAME} copy_shaders)

# Print status
message(STATUS "Step 2: Velocity field and advection")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
