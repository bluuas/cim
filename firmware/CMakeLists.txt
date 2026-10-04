cmake_minimum_required(VERSION 3.13)

# Board selection: cim_proto_v7 (RP2040) or cim_v0 (RP2354A).
# The board header sets PICO_PLATFORM accordingly.
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)
if (NOT DEFINED PICO_BOARD)
    set(PICO_BOARD cim_v0)
endif()

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(cim C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.1.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.1.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

pico_sdk_init()

add_compile_options(-Wall -Wextra)

add_subdirectory(util)
add_subdirectory(hal)
add_subdirectory(config)
add_subdirectory(commission)
add_subdirectory(drivers)
add_subdirectory(examples)
