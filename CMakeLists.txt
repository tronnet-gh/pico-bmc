cmake_minimum_required(VERSION 3.13)
set(PROGRAM_NAME pico_bmc)
set(PICO_BOARD pico_w)
include(pico_sdk_import.cmake)
project(pico_bmc)
pico_sdk_init()
add_executable(${PROGRAM_NAME}
    main.c
)
target_include_directories(${PROGRAM_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(${PROGRAM_NAME}
    pico_cyw43_arch_lwip_threadsafe_background
    pico_stdlib
    pico_multicore
    hardware_adc
)
pico_enable_stdio_usb(${PROGRAM_NAME} TRUE)
pico_enable_stdio_uart(${PROGRAM_NAME} FALSE)
pico_add_extra_outputs(${PROGRAM_NAME})