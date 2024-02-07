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
  pico_cyw43_arch_lwip_threadsafe_background
)
pico_enable_stdio_usb(bmc 1)
pico_enable_stdio_uart(bmc 0)
pico_add_extra_outputs(bmc)
target_include_directories(bmc PRIVATE ${CMAKE_CURRENT_LIST_DIR})