cmake_minimum_required(VERSION 3.7)
project(WireGuardKeyMine)

link_libraries(sodium m pthread)

option(ENABLE_O64 "Enable optimization for 64bit systems [speedup ~1.2%%]" ON)

set(CMAKE_C_STANDARD 99)
set(SOURCE_FILES main.c)
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

if(ENABLE_O64)
  add_definitions(-DO64)
endif()

add_executable(wgmine ${SOURCE_FILES})