cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project(SpookyScoreboardDaemon)

file(GLOB GAME_SOURCES src/game/*.cpp)

# vcpkg packages
find_package(jsoncpp CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_executable(ssbd
  main.cpp
  src/CurlHandler.cpp
  src/GameBase.cpp
  ${GAME_SOURCES}
)

target_link_directories(ssbd PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-linux/lib
)

target_include_directories(ssbd PRIVATE
  ${jsoncpp_INCLUDE_DIRS}
  ${yaml-cpp_INCLUDE_DIRS}
)

target_link_libraries(ssbd PRIVATE
  jsoncpp
  yaml-cpp::yaml-cpp
  curl
)
