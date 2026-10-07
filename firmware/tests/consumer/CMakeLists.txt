# A minimal project that uses CIM from outside, the way an application
# repository with CIM as a git submodule does. Built in CI to keep that
# path working:
#
#   cmake -S firmware/tests/consumer -B build/consumer -G Ninja -DPICO_BOARD=cim_proto_v7
#   cmake --build build/consumer
#
# In a real project, CIM_DIR is the submodule, e.g. ${CMAKE_CURRENT_LIST_DIR}/cim.
cmake_minimum_required(VERSION 3.13)

set(CIM_DIR ${CMAKE_CURRENT_LIST_DIR}/../../.. CACHE PATH "CIM repository")
include(${CIM_DIR}/firmware/cmake/cim_import.cmake)

project(cim_consumer C CXX ASM)

cim_init()

add_subdirectory(apps/hello)
