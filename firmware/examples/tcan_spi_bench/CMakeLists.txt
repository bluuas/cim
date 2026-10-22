# Two builds of the same code: from flash (XIP) and copied to RAM at start-up.
foreach(name tcan_spi_bench tcan_spi_bench_ram)
    add_executable(${name} main.c)
    target_link_libraries(${name} pico_stdlib cim_tcan4x5x)
    pico_enable_stdio_usb(${name} 1)
    pico_enable_stdio_uart(${name} 0)
    pico_add_extra_outputs(${name})
endforeach()
pico_set_binary_type(tcan_spi_bench_ram copy_to_ram)
