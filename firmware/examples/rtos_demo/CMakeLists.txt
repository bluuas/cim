add_executable(rtos_demo main.c)

target_link_libraries(rtos_demo cim_rtos cim_hal cim_log pico_stdlib)

pico_enable_stdio_usb(rtos_demo 1)
pico_enable_stdio_uart(rtos_demo 0)

pico_add_extra_outputs(rtos_demo)
