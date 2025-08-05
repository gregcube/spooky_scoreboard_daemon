cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SpookyScoreboardDaemon)
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-linux" ${CMAKE_PREFIX_PATH})
find_package(jsoncpp CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

file(GLOB GAME_SOURCES src/game/*.cpp)

add_executable(ssbd
  src/main.cpp
  src/x11.cpp
  src/CurlHandler.cpp
  src/QrCode.cpp
  src/QrScanner.cpp
  src/GameBase.cpp
  ${GAME_SOURCES}
)

target_include_directories(ssbd PRIVATE
  src/include
  /usr/include/freetype2
)

target_link_libraries(ssbd PRIVATE
  jsoncpp_static
  yaml-cpp::yaml-cpp
  curl
  X11
  Xft
  Xpm
  fontconfig
  pthread
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_compile_definitions(DEBUG)
endif()
