cmake_minimum_required(VERSION 3.8)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
set(CMAKE_BUILD_TYPE Debug)

project(SpookyScoreboardDaemon)

file(GLOB GAME_SOURCES src/game/*.cpp)

# vcpkg packages
find_package(jsoncpp CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_definitions(-DDEBUG)

add_executable(ssbd
  main.cpp
  src/CurlHandler.cpp
  src/QrCode.cpp
  src/GameBase.cpp
  ${GAME_SOURCES}
)

set(JSONCPP_STATIC_LIB "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-linux/lib/libjsoncpp.a")
set(YAML_CPP_STATIC_LIB "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-linux/lib/libyaml-cpp.a")

target_include_directories(ssbd PRIVATE
  ${jsoncpp_INCLUDE_DIRS}
  ${yaml-cpp_INCLUDE_DIRS}
  /usr/include/freetype2
  ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-linux/include
)

target_link_libraries(ssbd PRIVATE
  ${JSONCPP_STATIC_LIB}
  ${YAML_CPP_STATIC_LIB}
  curl
  X11
  Xft
  Xpm
  fontconfig
  pthread
)
