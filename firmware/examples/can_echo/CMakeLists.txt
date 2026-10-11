add_executable(can_echo main.c)

target_link_libraries(can_echo pico_stdlib cim_tcan4x5x)

pico_enable_stdio_usb(can_echo 1)
pico_enable_stdio_uart(can_echo 0)

pico_add_extra_outputs(can_echo)
