project(nativethread C)
cmake_minimum_required(VERSION 2.6)
include(dist.cmake)
include(checkit.cmake)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(deps/uv)
include_directories(deps/uv/include)

install_lua_module(nativethread
  src/nativethread.c
)
add_checkit_test(test/*.lua)
target_link_libraries(nativethread uv pthread rt)
