# FreeRTOS

`cim_rtos` brings FreeRTOS to CIM applications: the kernel with `heap_4`, the CIM configuration, hooks, and a `cim_delay_ms()` that blocks the calling task instead of busy-waiting.

```cmake
target_link_libraries(my_app cim_rtos)
```

```c
#include "FreeRTOS.h"
#include "task.h"

xTaskCreate(app_task, "app", 2 * configMINIMAL_STACK_SIZE, NULL, 2, NULL);
vTaskStartScheduler();
```

See [examples/rtos_demo](../examples/rtos_demo/main.c).

## Kernel

| | |
|---|---|
| Source | [raspberrypi/FreeRTOS-Kernel](https://github.com/raspberrypi/FreeRTOS-Kernel), git submodule `FreeRTOS-Kernel` |
| Version | V11.1.0+, commit `4f7299d` (the fork has no tags) |
| Ports | `RP2040` (proto v7), `RP2350_ARM_NTZ` (v0), chosen from `PICO_PLATFORM` |
| Why the fork | upstream FreeRTOS (V11.3.1) has the RP2040 port, but no RP2350 port yet |

After cloning: `git submodule update --init`.

## Configuration

[`config/FreeRTOSConfig.h`](config/FreeRTOSConfig.h) is shared by all applications. An application can override these values in its `app_config.h` (on its include path) or as compile definitions:

| Setting | Default | |
|---|---|---|
| `configTICK_RATE_HZ` | 1000 | 1 ms tick |
| `configMAX_PRIORITIES` | 8 | timer task runs at the highest |
| `configMINIMAL_STACK_SIZE` | 256 | words (1 KB) |
| `configTOTAL_HEAP_SIZE` | 64 KB | `heap_4` |
| `configNUMBER_OF_CORES` | 1 | FreeRTOS on core 0 only; 2 enables SMP |
| `configTIMER_TASK_STACK_DEPTH` | 512 | words |

The Pico SDK interop is on (`configSUPPORT_PICO_SYNC_INTEROP`, `configSUPPORT_PICO_TIME_INTEROP`): SDK mutexes, `sleep_ms()` and stdio over USB work together with the scheduler.

## Hooks and delays

| | Behaviour |
|---|---|
| `cim_delay_ms()` | `vTaskDelay()` (rounded up to whole ticks) while the scheduler runs, `sleep_ms()` before. Drivers using the HAL, e.g. the TCAN4x5x driver, wait without blocking other tasks. |
| out of heap | logs an error (`cim/log.h`) and stops with `panic()` |
| stack overflow | checked on every context switch (method 2), logs the task name and stops with `panic()` |

`cim_rtos` is an INTERFACE library: `src/rtos_hooks.c` is compiled into each application, so its `cim_delay_ms()` replaces the weak busy-waiting one in `cim_hal`.

## Test

[`hil/runner/test_rtos.py`](../../hil/runner/test_rtos.py) runs `rtos_demo` on the bench: task rates, the 1 kHz tick, and a low-priority task that only gets CPU time because `cim_delay_ms()` blocks.
