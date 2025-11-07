cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SpookyScoreboardDaemon)
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux" ${CMAKE_PREFIX_PATH})
find_package(jsoncpp CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)

file(GLOB GAME_SOURCES src/game/*.cpp)
file(GLOB GAME_INCLUDES src/game/*.h)
file(GLOB FONT_SOURCES src/font/*.h)

add_executable(ssbd
  src/main.cpp
  src/x11.cpp
  src/CurlHandler.cpp
  src/QrCode.cpp
  src/QrScanner.cpp
  src/GameBase.cpp
  ${GAME_SOURCES}
  ${GAME_INCLUDES}
  ${FONT_SOURCES}
)

target_include_directories(ssbd PRIVATE
  src/
  /usr/include/freetype2
)

target_link_libraries(ssbd PRIVATE
  jsoncpp_static
  yaml-cpp::yaml-cpp
  CURL::libcurl
  X11
  Xft
  Xpm
  fontconfig
  pthread
  dl
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_compile_definitions(DEBUG)
endif()
