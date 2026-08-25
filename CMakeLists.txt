cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SpookyScoreboardDaemon)
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux" ${CMAKE_PREFIX_PATH})

find_package(Threads REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED)

file(GLOB GAME_SOURCES src/game/*.cpp)
file(GLOB GAME_INCLUDES src/game/*.h)
file(GLOB FONT_SOURCES src/font/*.h)

add_executable(ssbd
  src/main.cpp
  src/x11.cpp
  src/QrCode.cpp
  src/QrScanner.cpp
  src/GameBase.cpp
  src/WebSocket.cpp
  src/Register.cpp
  src/Player.cpp
  src/Config.cpp
  src/Signature.cpp
  src/MessageQueue.cpp
  src/Journal.cpp
  src/AuditEvent.cpp
  ${GAME_SOURCES}
  ${GAME_INCLUDES}
  ${FONT_SOURCES}
)

target_include_directories(ssbd PRIVATE
  src/
  /usr/include/freetype2
)

target_link_libraries(ssbd PRIVATE
  Threads::Threads
  jsoncpp_static
  yaml-cpp::yaml-cpp
  ixwebsocket::ixwebsocket
  X11
  Xft
  Xpm
  fontconfig
  pthread
  uuid
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_compile_definitions(DEBUG)
endif()

set(SSB_ENV "live" CACHE STRING "Target environment: local, stage, live")
set_property(CACHE SSB_ENV PROPERTY STRINGS local stage live)

if(SSB_ENV STREQUAL "local")
  set(SSB_WS_URL "wss://ssb.local:4444")
elseif(SSB_ENV STREQUAL "stage")
  set(SSB_WS_URL "wss://stage.spookyscoreboard.com:4444")
elseif(SSB_ENV STREQUAL "live")
  set(SSB_WS_URL "wss://spookyscoreboard.com:4444")
else()
  message(FATAL_ERROR "Unknown SSB_ENV=${SSB_ENV} (expected local, stage, or live)")
endif()

target_compile_definitions(ssbd PRIVATE
  "SSB_ENV=\"${SSB_ENV}\""
  "SSB_WS_URL=\"${SSB_WS_URL}\""
)

message(STATUS "SSB_ENV=${SSB_ENV} SSB_WS_URL=${SSB_WS_URL}")
