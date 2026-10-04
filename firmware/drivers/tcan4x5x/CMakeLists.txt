# TCAN4x5x driver: TI's driver library (unmodified, ti/) plus the CIM SPI port (port/).
add_library(cim_tcan4x5x STATIC
    ti/TCAN4550.c
    port/TCAN4x5x_SPI.c
)

# SYSTEM: don't report warnings from TI's headers in code that includes them
target_include_directories(cim_tcan4x5x SYSTEM PUBLIC ti)
target_include_directories(cim_tcan4x5x PUBLIC port)

target_link_libraries(cim_tcan4x5x PUBLIC cim_hal)

# TI's TCAN4x5x_MCAN_ReadNextFIFO()/ReadRXBuffer() return an uninitialised byte
# count for zero-length frames (DLC 0). TI's code is kept unmodified; callers must
# use the DLC from the RX header instead of the return value. See README.md and #34.
set_source_files_properties(ti/TCAN4550.c PROPERTIES COMPILE_OPTIONS "-Wno-maybe-uninitialized")
