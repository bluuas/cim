add_executable(log_demo main.c)

target_link_libraries(log_demo pico_stdlib cim_log)

pico_enable_stdio_usb(log_demo 1)
pico_enable_stdio_uart(log_demo 0)

pico_add_extra_outputs(log_demo)
