cmake_minimum_required(VERSION 3.10)
project(LBM_Complete)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# NO boundary shaders needed!
set(SHADER_NAMES
    lbm_init
    lbm_collision
    lbm_streaming
    lbm_macro
    lbm_display
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
