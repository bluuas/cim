# FreeRTOS for CIM applications (cim_rtos).
#
# The kernel is the raspberrypi/FreeRTOS-Kernel fork (git submodule, pinned),
# because upstream FreeRTOS has no RP2350 port yet. Applications link cim_rtos;
# it brings the kernel with heap_4, the CIM FreeRTOSConfig.h and the hooks.

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Kernel)
if (NOT EXISTS ${FREERTOS_KERNEL_PATH}/tasks.c)
    message(FATAL_ERROR "FreeRTOS-Kernel not found: run 'git submodule update --init'")
endif()

if (PICO_PLATFORM STREQUAL "rp2040")
    set(_port RP2040)
else()
    set(_port RP2350_ARM_NTZ)
endif()
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/${_port}/FreeRTOS_Kernel_import.cmake)

# INTERFACE: the hooks are compiled into each application, so its strong
# cim_delay_ms() replaces the weak one in cim_hal.
add_library(cim_rtos INTERFACE)

target_sources(cim_rtos INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src/rtos_hooks.c)

target_include_directories(cim_rtos INTERFACE ${CMAKE_CURRENT_LIST_DIR}/config)

# FreeRTOS runs on core 0 only (configNUMBER_OF_CORES 1), core 1 is unused.
# Tell flash_safe_execute() (config store writes) that it only has to disable
# interrupts. Without this, the port's FREE_RTOS_KERNEL_SMP makes it try to
# stop core 1, which fails: assert in debug builds, error in release builds.
target_compile_definitions(cim_rtos INTERFACE PICO_FLASH_ASSUME_CORE1_SAFE=1)

target_link_libraries(cim_rtos INTERFACE FreeRTOS-Kernel FreeRTOS-Kernel-Heap4 cim_hal cim_log pico_stdlib)
