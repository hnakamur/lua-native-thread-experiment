project(nativethread C)
cmake_minimum_required(VERSION 2.6)
include(dist.cmake)
include(checkit.cmake)

install_lua_module(nativethread
  src/nativethread.c
)
add_checkit_test(test/*.lua)
target_link_libraries(nativethread pthread)
