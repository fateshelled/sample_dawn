cmake_minimum_required(VERSION 3.22)
project(sample_dawn)
set(CMAKE_CXX_STANDARD 20)

set(DAWN_FETCH_DEPENDENCIES ON)
set(DAWN_USE_GLFW ON)
set(DAWN_ENABLE_INSTALL ON)

add_subdirectory("dawn" EXCLUDE_FROM_ALL)

# GLFW includes and libraries
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dawn/third_party/glfw")
include_directories("${GLFW_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dawn/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/dawn/gen/include")

add_executable(device_query 
    device_query.cpp 
)
target_link_libraries(device_query PRIVATE 
    webgpu_dawn
)

add_executable(ply_viewer 
    main.cpp 
    PlyLoader.cpp 
    Renderer.cpp
)

target_link_libraries(ply_viewer PRIVATE 
    webgpu_dawn 
    dawn_native 
    webgpu_glfw
    glfw
    X11
)

target_compile_definitions(ply_viewer PRIVATE 
    WGPU_SHARED_LIBRARY
)
