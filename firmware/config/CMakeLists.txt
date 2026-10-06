# Persistent key/value configuration in flash (cim/config.h).
add_library(cim_config STATIC
    src/config.c
)

target_include_directories(cim_config PUBLIC include)

target_link_libraries(cim_config PUBLIC cim_hal cim_util)
