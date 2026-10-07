# Use the CIM platform in a CMake project: this repository, or another one
# that has CIM as a git submodule (e.g. at cim/):
#
#   cmake_minimum_required(VERSION 3.13)
#   include(cim/firmware/cmake/cim_import.cmake)   # before project()
#   project(my_project C CXX ASM)
#   cim_init()                                     # after project()
#   add_subdirectory(apps/my_app)                  # uses cim_add_app()
#
# Board: -DPICO_BOARD=cim_proto_v7 (RP2040) or cim_v0 (RP2354A, default).
# The Pico SDK is found like the SDK's own pico_sdk_import.cmake does
# (PICO_SDK_PATH, or PICO_SDK_FETCH_FROM_GIT).

include_guard(GLOBAL)

get_filename_component(CIM_FIRMWARE_DIR ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)

# Board headers: CIM's boards; a project can append its own directories.
list(APPEND PICO_BOARD_HEADER_DIRS ${CIM_FIRMWARE_DIR}/boards)
if (NOT DEFINED PICO_BOARD)
    set(PICO_BOARD cim_v0)
endif()

include(${CIM_FIRMWARE_DIR}/pico_sdk_import.cmake)

# Initialise the Pico SDK and add the CIM libraries. A macro, because
# pico_sdk_init() must run in the scope of the calling project.
macro(cim_init)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    if (PICO_SDK_VERSION_STRING VERSION_LESS "2.1.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK 2.1.0 or later required, found ${PICO_SDK_VERSION_STRING}")
    endif()

    pico_sdk_init()

    add_compile_options(-Wall -Wextra)

    foreach(_cim_lib util hal config log rtos commission drivers)
        add_subdirectory(${CIM_FIRMWARE_DIR}/${_cim_lib} ${CMAKE_BINARY_DIR}/cim/${_cim_lib})
    endforeach()
endmacro()

# Add a CIM application:
#
#   cim_add_app(<name> SOURCES main.c app.c [LIBS <extra libraries>] [NO_RTOS])
#
# Links the CIM platform (HAL, config, log, commissioning, TCAN4x5x driver)
# and, unless NO_RTOS is given, FreeRTOS (cim_rtos). The app's directory is
# on the include path, so its app_config.h can override FreeRTOSConfig.h
# values. Enables stdio over USB and writes .uf2/.elf/.bin.
function(cim_add_app name)
    cmake_parse_arguments(APP "NO_RTOS" "" "SOURCES;LIBS" ${ARGN})
    if (NOT APP_SOURCES)
        message(FATAL_ERROR "cim_add_app(${name}): no SOURCES given")
    endif()
    if (APP_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "cim_add_app(${name}): unknown arguments ${APP_UNPARSED_ARGUMENTS}")
    endif()

    add_executable(${name} ${APP_SOURCES})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${name} PRIVATE
        pico_stdlib
        cim_hal
        cim_util
        cim_config
        cim_log
        cim_commission
        cim_tcan4x5x
        ${APP_LIBS}
    )
    if (NOT APP_NO_RTOS)
        target_link_libraries(${name} PRIVATE cim_rtos)
    endif()

    pico_enable_stdio_usb(${name} 1)
    pico_enable_stdio_uart(${name} 0)
    pico_add_extra_outputs(${name})
endfunction()
