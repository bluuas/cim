# Version of the CIM platform (cim/version.h), generated at build time.
# Override with -DCIM_VERSION=<string>.
set(CIM_VERSION "" CACHE STRING "Version string instead of git describe")

set(_version_c ${CMAKE_CURRENT_BINARY_DIR}/cim_version.c)
add_custom_target(cim_version_generate
    COMMAND ${CMAKE_COMMAND}
        -DOUT=${_version_c}
        -DREPO=${CIM_FIRMWARE_DIR}
        -DCIM_VERSION=${CIM_VERSION}
        -P ${CMAKE_CURRENT_LIST_DIR}/generate.cmake
    BYPRODUCTS ${_version_c}
    COMMENT "Generating the CIM version"
)

add_library(cim_version STATIC ${_version_c})
add_dependencies(cim_version cim_version_generate)
target_include_directories(cim_version PUBLIC include)
