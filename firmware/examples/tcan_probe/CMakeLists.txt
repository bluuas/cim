add_executable(tcan_probe main.c)

target_link_libraries(tcan_probe pico_stdlib cim_hal)

pico_enable_stdio_usb(tcan_probe 1)
pico_enable_stdio_uart(tcan_probe 0)

pico_add_extra_outputs(tcan_probe)
