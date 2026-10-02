add_library(cim_hal STATIC
    src/hal_delay.c
    src/hal_gpio.c
    src/hal_spi.c
)

target_include_directories(cim_hal PUBLIC include)

target_link_libraries(cim_hal PUBLIC
    hardware_gpio
    hardware_spi
    hardware_sync
    pico_time
)
