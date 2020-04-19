cmake_minimum_required(VERSION 3.15)
project(gl_sandbox)


find_package(glm REQUIRED)
find_package(assimp REQUIRED)

add_subdirectory(third/glad)
add_subdirectory(third/glfw)
add_subdirectory(third/assimp)
add_subdirectory(third/tinygltf)
set(CMAKE_CXX_STANDARD 20)

file(GLOB src "./*.cpp")
file(GLOB gl_src "./gl/*.cpp")

include_directories(${CMAKE_CURRENT_LIST_DIR})
add_executable(gl_sandbox main.cpp ${src} ${gl_src})

target_link_libraries(gl_sandbox glm assimp glad glfw tinygltf)