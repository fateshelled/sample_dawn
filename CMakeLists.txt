cmake_minimum_required(VERSION 3.22)
project(sample_dawn)
set(CMAKE_CXX_STANDARD 20)

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)

add_executable(device_query "device_query.cpp")
target_link_libraries(device_query PRIVATE webgpu_dawn)
