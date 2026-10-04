add_executable(config_counter main.c)

target_link_libraries(config_counter pico_stdlib cim_config)

pico_enable_stdio_usb(config_counter 1)
pico_enable_stdio_uart(config_counter 0)

pico_add_extra_outputs(config_counter)
