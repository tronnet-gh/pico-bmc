cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(bmc)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_executable(bmc
  bmc.c
)
target_link_libraries(bmc
  pico_stdlib
  pico_cyw43_arch_none
)
pico_add_extra_outputs(bmc)