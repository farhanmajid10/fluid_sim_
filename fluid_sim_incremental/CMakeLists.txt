cmake_minimum_required(VERSION 3.10)
project(IncrementalLBM_Step3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)  # Debug for now to get better error info
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

find_package(OpenGL REQUIRED)
add_subdirectory(lib/flgl)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/lib/flgl/inc
    ${CMAKE_SOURCE_DIR}/lib/flgl/lib/glad/include
    ${CMAKE_SOURCE_DIR}/lib/flgl/lib/glfw/include
    ${CMAKE_SOURCE_DIR}/lib/flgl/lib/glm
    ${CMAKE_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIRS}
)   

target_link_libraries(${PROJECT_NAME} PRIVATE 
    FLGL
    ${OPENGL_LIBRARIES}
)

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ${X11_LIBRARIES}
        ${CMAKE_DL_LIBS}
        Threads::Threads
    )
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib/flgl/res/default_shaders)

set(SHADER_NAMES
    lbm_init3
    lbm_macro3
    display3
)

foreach(SHADER_NAME ${SHADER_NAMES})
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

add_custom_target(copy_shaders ALL
    COMMENT "Copying Step 3 shaders"
)

add_dependencies(${PROJECT_NAME} copy_shaders)

message(STATUS "Step 3: LBM Distribution Functions - Debug Build")
