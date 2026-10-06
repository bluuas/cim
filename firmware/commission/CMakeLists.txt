# Commissioning over USB serial (cim/commission.h).
add_library(cim_commission STATIC
    src/commission.c
    src/commission_pico.c
)

target_include_directories(cim_commission PUBLIC include)

target_link_libraries(cim_commission PUBLIC
    cim_config
    hardware_watchdog
    pico_bootrom
    pico_stdlib
    pico_unique_id
)
