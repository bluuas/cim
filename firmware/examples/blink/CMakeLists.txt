add_executable(blink main.c)

target_link_libraries(blink pico_stdlib)

# stdio over USB CDC (GPIO0/1 are not available for a UART)
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 0)

pico_add_extra_outputs(blink)
