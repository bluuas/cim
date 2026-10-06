add_executable(commission main.c)

target_link_libraries(commission pico_stdlib cim_commission)

pico_enable_stdio_usb(commission 1)
pico_enable_stdio_uart(commission 0)

pico_add_extra_outputs(commission)
