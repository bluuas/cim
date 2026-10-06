# Platform-independent helpers (also built for host tests).
add_library(cim_util STATIC
    src/crc32.c
)

target_include_directories(cim_util PUBLIC include)
