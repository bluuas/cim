/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * FreeRTOS configuration for CIM applications (RP2040 and RP2350, ARM).
 *
 * An application can override the values marked "overridable" by defining
 * them in its app_config.h (found on the include path) or as compile
 * definitions. Everything else is fixed by the platform.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#if __has_include("app_config.h")
#include "app_config.h"
#endif

/* ------------------------------ overridable ------------------------------- */

#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ 1000
#endif

#ifndef configMAX_PRIORITIES
#define configMAX_PRIORITIES 8
#endif

/** Stack of the idle task and the minimum for other tasks, in words (4 bytes). */
#ifndef configMINIMAL_STACK_SIZE
#define configMINIMAL_STACK_SIZE 256
#endif

#ifndef configTOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE (64 * 1024)
#endif

/** 1: FreeRTOS on core 0 only (default). 2: SMP on both cores. */
#ifndef configNUMBER_OF_CORES
#define configNUMBER_OF_CORES 1
#endif

#ifndef configTIMER_TASK_STACK_DEPTH
#define configTIMER_TASK_STACK_DEPTH 512
#endif

/* -------------------------------- scheduler ------------------------------- */

#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configIDLE_SHOULD_YIELD                 1
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS
#define configMAX_TASK_NAME_LEN                 16
#define configSTACK_DEPTH_TYPE                  uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5
#define configENABLE_BACKWARD_COMPATIBILITY     0

#if configNUMBER_OF_CORES > 1
#define configUSE_CORE_AFFINITY      1
#define configRUN_MULTIPLE_PRIORITIES 1
#define configUSE_PASSIVE_IDLE_HOOK  0
#endif

/* -------------------------- synchronisation, IPC -------------------------- */

#define configUSE_MUTEXES             1
#define configUSE_RECURSIVE_MUTEXES   1
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_QUEUE_SETS          1
#define configQUEUE_REGISTRY_SIZE     8
#define configUSE_TASK_NOTIFICATIONS  1

#define configUSE_TIMERS             1
#define configTIMER_TASK_PRIORITY    (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH     10

/* --------------------------------- memory --------------------------------- */

#define configSUPPORT_STATIC_ALLOCATION  0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configAPPLICATION_ALLOCATED_HEAP 0

/* ---------------------------- hooks, debugging ---------------------------- */

#define configUSE_IDLE_HOOK                  0
#define configUSE_TICK_HOOK                  0
#define configUSE_MALLOC_FAILED_HOOK         1 /* rtos_hooks.c: log + panic */
#define configCHECK_FOR_STACK_OVERFLOW       2 /* rtos_hooks.c: log + panic */
#define configUSE_TRACE_FACILITY             1
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define configGENERATE_RUN_TIME_STATS        0
#define configUSE_CO_ROUTINES                0

#include <assert.h>
#define configASSERT(x) assert(x)

/* ------------------------------- Pico SDK --------------------------------- */

/* SDK mutexes/semaphores and sleep_ms() cooperate with the scheduler */
#define configSUPPORT_PICO_SYNC_INTEROP 1
#define configSUPPORT_PICO_TIME_INTEROP 1

#if PICO_RP2350
#define configENABLE_MPU                     0
#define configENABLE_TRUSTZONE               0
#define configRUN_FREERTOS_SECURE_ONLY       1
#define configENABLE_FPU                     1
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 16
#endif

/* --------------------------- optional functions --------------------------- */

#define INCLUDE_vTaskPrioritySet             1
#define INCLUDE_uxTaskPriorityGet            1
#define INCLUDE_vTaskDelete                  1
#define INCLUDE_vTaskSuspend                 1
#define INCLUDE_xTaskDelayUntil              1
#define INCLUDE_vTaskDelay                   1
#define INCLUDE_xTaskGetSchedulerState       1
#define INCLUDE_xTaskGetCurrentTaskHandle    1
#define INCLUDE_uxTaskGetStackHighWaterMark  1
#define INCLUDE_xTaskGetIdleTaskHandle       1
#define INCLUDE_eTaskGetState                1
#define INCLUDE_xTimerPendFunctionCall       1
#define INCLUDE_xTaskAbortDelay              1
#define INCLUDE_xTaskGetHandle               1
#define INCLUDE_xTaskResumeFromISR           1
#define INCLUDE_xQueueGetMutexHolder         1

#endif /* FREERTOS_CONFIG_H */
