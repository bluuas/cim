add_executable(can_load main.c)

target_link_libraries(can_load pico_stdlib cim_tcan4x5x)

pico_enable_stdio_usb(can_load 1)
pico_enable_stdio_uart(can_load 0)

pico_add_extra_outputs(can_load)

# The board with the defective hardware divider (#41, bench slot C) computes
# wrong bit timing at 8 Mbit/s with it; the software divider makes both ends usable.
pico_set_divider_implementation(can_load compiler)
