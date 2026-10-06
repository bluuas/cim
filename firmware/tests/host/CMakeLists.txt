# Host unit tests: platform-independent firmware code, built with the PC compiler.
#
#   cmake -S firmware/tests/host -B build/host-tests
#   cmake --build build/host-tests && ctest --test-dir build/host-tests --output-on-failure
cmake_minimum_required(VERSION 3.13)
project(cim_host_tests C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -Werror -fsanitize=address,undefined -g)
add_link_options(-fsanitize=address,undefined)

set(FW ${CMAKE_CURRENT_LIST_DIR}/../..)

enable_testing()

add_executable(test_config
    test_config.c
    flash_mock.c
    ${FW}/config/src/config.c
    ${FW}/util/src/crc32.c
)
target_include_directories(test_config PRIVATE
    .
    ${FW}/hal/include
    ${FW}/config/include
    ${FW}/util/include
)
add_test(NAME config COMMAND test_config)

add_executable(test_commission
    test_commission.c
    flash_mock.c
    ${FW}/commission/src/commission.c
    ${FW}/config/src/config.c
    ${FW}/util/src/crc32.c
)
target_include_directories(test_commission PRIVATE
    .
    ${FW}/hal/include
    ${FW}/config/include
    ${FW}/commission/include
    ${FW}/util/include
)
add_test(NAME commission COMMAND test_commission)
