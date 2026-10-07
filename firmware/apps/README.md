# Applications

Every CIM application has the same structure, whether it lives in this repository or in another one that uses CIM as a git submodule.

```
apps/<name>/
  CMakeLists.txt   cim_add_app(<name> SOURCES main.c app.c)
  main.c           platform start-up, then app_run()
  app.c, app.h     the application: tasks, APP_NAME, APP_VERSION
  app_config.h     build-time settings (optional), also read by FreeRTOSConfig.h
```

[`example/`](example/) is the template: copy it, rename it and add it to `apps/CMakeLists.txt`.

## cim_add_app()

```cmake
cim_add_app(<name> SOURCES main.c app.c [LIBS <extra libraries>] [NO_RTOS])
```

| | |
|---|---|
| links | Pico SDK (`pico_stdlib`), CIM HAL, config store, logging, commissioning, TCAN4x5x driver, and FreeRTOS (`cim_rtos`) unless `NO_RTOS` |
| include path | the app's own directory, so `app_config.h` is found by `FreeRTOSConfig.h` |
| outputs | `.elf`, `.uf2`, `.bin`; stdio over USB on, UART off |

Applications use FreeRTOS by default. `NO_RTOS` is for special cases; the examples in [`../examples`](../examples) are bare metal on purpose, to test single modules.

`app_config.h` can override the FreeRTOS settings listed in [`../rtos/README.md`](../rtos/README.md#configuration), e.g. `#define configTOTAL_HEAP_SIZE (32 * 1024)`.

## Using CIM as a submodule

An application repository (e.g. a team's car-specific apps) adds CIM as a git submodule and keeps the same app structure:

```
my-apps/
  cim/                  git submodule: this repository
  CMakeLists.txt
  apps/
    ebs/  drs/ ...      same files as apps/example
```

```sh
git submodule add https://github.com/bluuas/cim.git cim
git submodule update --init --recursive    # CIM has FreeRTOS as a submodule itself
```

`CMakeLists.txt` of the application repository:

```cmake
cmake_minimum_required(VERSION 3.13)

include(cim/firmware/cmake/cim_import.cmake)   # before project(): board, Pico SDK
project(my_apps C CXX ASM)
cim_init()                                     # after project(): Pico SDK, CIM libraries

add_subdirectory(apps/ebs)
add_subdirectory(apps/drs)
```

Build with the board selected (`cim_v0` is the default):

```sh
cmake -S . -B build -G Ninja -DPICO_BOARD=cim_proto_v7
cmake --build build
```

The Pico SDK is found through `PICO_SDK_PATH`, as in this repository; the CIM devcontainer provides it. [`../tests/consumer`](../tests/consumer) is a minimal project of this kind, and CI builds it.

To update CIM: `git -C cim pull` (or check out a release tag), then commit the new submodule commit.
