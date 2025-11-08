cmake_minimum_required(VERSION 3.10)
project(LBM_FluidSim)

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

# Link libraries - FIXED: Changed flgl to FLGL
target_link_libraries(${PROJECT_NAME} PRIVATE 
    FLGL
    ${OPENGL_LIBRARIES}
)

# Platform-specific settings
if(WIN32)
    # Windows specific settings
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
elseif(APPLE)
    # macOS specific settings
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
elseif(UNIX)
    # Linux specific settings
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ${X11_LIBRARIES}
        ${CMAKE_DL_LIBS}
        Threads::Threads
    )
endif()

# Copy shaders to build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

# List of shader files
set(SHADER_FILES
    lbm_init.vert
    lbm_init.frag
    lbm_collision.vert
    lbm_collision.frag
    lbm_streaming.vert
    lbm_streaming.frag
    lbm_boundary.vert
    lbm_boundary.frag
    lbm_mouse.vert
    lbm_mouse.frag
    lbm_macroscopic.vert
    lbm_macroscopic.frag
    visualization.vert
    visualization.frag
)

# Copy each shader file
foreach(SHADER ${SHADER_FILES})
    configure_file(
        ${CMAKE_SOURCE_DIR}/shaders/${SHADER}
        ${CMAKE_BINARY_DIR}/shaders/${SHADER}
        COPYONLY
    )
endforeach()

# Add custom target for shader copying
add_custom_target(copy_shaders ALL
    COMMENT "Copying shader files to build directory"
)

# Make executable depend on shader copying
add_dependencies(${PROJECT_NAME} copy_shaders)

# Installation rules (optional)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY shaders/ DESTINATION bin/shaders)
