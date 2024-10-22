include(FetchContent)
cmake_minimum_required(VERSION 3.18.0 FATAL_ERROR)
project(gpupro LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

FetchContent_Declare(
  tga
  GIT_REPOSITORY git@gitlab.lrz.de:tuini15/gpu-pro/public/TGA.git
  GIT_TAG        3d75c12151e1a4f1e4d9acc0ae52cb1f4b27e092
)

FetchContent_MakeAvailable(tga)

add_executable(gpupro src/main.cpp)
target_link_libraries(gpupro PRIVATE tga_vulkan PRIVATE tga_utils)
