set(SRCS main.c)

if(CMAKE_HOST_APPLE)
  find_path(
    SDL_INCLUDE
    NAMES SDL2
    PATHS "/opt/homebrew/include"
    NO_DEFAULT_PATH)
  find_library(
    SDL_LIBRARY
    NAMES SDL2 SDL2main
    PATHS "/opt/homebrew/lib"
    NO_DEFAULT_PATH)
else()
  find_path(
    SDL_INCLUDE
    NAMES SDL.h
    PATHS "/usr/include"
    NO_DEFAULT_PATH)
  find_library(
    SDL_LIBRARY
    NAMES SDL2 SDL2main
    PATHS "/usr/lib64"
    NO_DEFAULT_PATH)
endif()

if(NOT SDL_LIBRARY)
  message(WARNING "SDL2 Library Not Found")
endif()

add_executable(CHIP-8 ${SRCS})

target_compile_options(
  CHIP-8
  PRIVATE -O3
          -m64
          -march=native
          -mtune=native
          -Wall
          -Wextra
          -Werror
          -Wpedantic)

target_include_directories(CHIP-8 PRIVATE ${SDL_INCLUDE})

target_link_libraries(CHIP-8 PRIVATE ${SDL_LIBRARY})
