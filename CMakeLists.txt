cmake_minimum_required(VERSION 3.20)
project(CHIP-8 LANGUAGES C)

if(CMAKE_HOST_APPLE)
  set(CMAKE_C_COMPILER clang)
else()
  set(CMAKE_C_COMPILER gcc)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory("src")
