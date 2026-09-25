cmake_minimum_required(VERSION 3.20)
project(IsometricSnake VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 6.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_library(snake_core src/game.cpp)
target_include_directories(snake_core PUBLIC include)

add_executable(isometric_snake src/main.cpp)
target_link_libraries(isometric_snake PRIVATE snake_core raylib)

if(APPLE)
  target_link_libraries(isometric_snake PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(snake_tests tests/game_tests.cpp)
  target_link_libraries(snake_tests PRIVATE snake_core)
  add_test(NAME snake_core_tests COMMAND snake_tests)
endif()
