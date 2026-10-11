add_executable(can_tx_counter main.c)

target_link_libraries(can_tx_counter pico_stdlib cim_tcan4x5x)

pico_enable_stdio_usb(can_tx_counter 1)
pico_enable_stdio_uart(can_tx_counter 0)

pico_add_extra_outputs(can_tx_counter)
