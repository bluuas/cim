# Logging with levels and a compile-time filter (cim/log.h).
add_library(cim_log STATIC
    src/log.c
    src/log_pico.c
)

target_include_directories(cim_log PUBLIC include)

target_link_libraries(cim_log PUBLIC pico_stdio pico_time pico_sync hardware_sync)
