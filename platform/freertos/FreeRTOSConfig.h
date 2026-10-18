/**
 * Mac 上的 FreeRTOS 配置。用 POSIX 移植，任务是 pthread，系统节拍用信号。
 * 界面任务和逻辑任务靠阻塞切换，不做时间片轮转，避免 printf 被切走时卡住。
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION                       1
#define configUSE_TIME_SLICING                     0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    0
#define configUSE_IDLE_HOOK                        0
#define configUSE_TICK_HOOK                        0
#define configUSE_DAEMON_TASK_STARTUP_HOOK         0
#define configTICK_RATE_HZ                         1000
#define configTICK_TYPE_WIDTH_IN_BITS              TICK_TYPE_WIDTH_64_BITS
#define configMAX_PRIORITIES                       5
#define configMINIMAL_STACK_SIZE                   256
#define configMAX_TASK_NAME_LEN                    16
#define configUSE_TRACE_FACILITY                   0
#define configIDLE_SHOULD_YIELD                    1
#define configUSE_MUTEXES                          1
#define configUSE_RECURSIVE_MUTEXES                0
#define configUSE_COUNTING_SEMAPHORES              0
#define configUSE_TASK_NOTIFICATIONS               1
#define configCHECK_FOR_STACK_OVERFLOW             0
#define configUSE_MALLOC_FAILED_HOOK               0
#define configUSE_APPLICATION_TASK_TAG             0
#define configGENERATE_RUN_TIME_STATS              0
#define configQUEUE_REGISTRY_SIZE                  0
#define configSUPPORT_STATIC_ALLOCATION            0
#define configSUPPORT_DYNAMIC_ALLOCATION           1
#define configUSE_TIMERS                           0
#define configTOTAL_HEAP_SIZE                      (64 * 1024)
#define configNUMBER_OF_CORES                      1

#define INCLUDE_vTaskDelay                         1
#define INCLUDE_vTaskDelete                        1
#define INCLUDE_vTaskSuspend                       1
#define INCLUDE_xTaskGetCurrentTaskHandle          1
#define INCLUDE_xTaskGetSchedulerState             1

#define configASSERT(x)    do { if ((x) == 0) vAssertCalled(__FILE__, __LINE__); } while (0)

void vAssertCalled(const char* file, int line);

#endif
