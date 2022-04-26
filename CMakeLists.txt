cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(flashid C CXX ASM)

pico_sdk_init()

add_executable(flashid main.c)

pico_enable_stdio_uart(flashid 1)
pico_enable_stdio_usb(flashid 0)

target_link_libraries(flashid pico_stdlib hardware_flash)

pico_add_extra_outputs(flashid)
