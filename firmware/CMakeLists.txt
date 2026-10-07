cmake_minimum_required(VERSION 3.13)

# Same setup as a project that uses CIM as a submodule (cmake/cim_import.cmake).
# Board: -DPICO_BOARD=cim_proto_v7 (RP2040) or cim_v0 (RP2354A, default);
# the presets in CMakePresets.json set it.
include(cmake/cim_import.cmake)

project(cim C CXX ASM)

cim_init()

add_subdirectory(examples)
add_subdirectory(apps)
