#include "cmsis_os2.h"
#include "stm32h7xx_hal.h"

static uint32_t kernel_tick_count = 0;

osKernelState_t osKernelGetState(void) {
    return osKernelRunning;
}

osStatus_t osKernelInitialize(void) {
    return osOK;
}

osStatus_t osKernelStart(void) {
    return osOK;
}

uint32_t osKernelGetTickCount(void) {
    return kernel_tick_count;
}

uint32_t osKernelGetTickFreq(void) {
    return 1000U;
}

osStatus_t osDelay(uint32_t ticks) {
    HAL_Delay(ticks);
    return osOK;
}

osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr) {
    (void)func;
    (void)argument;
    (void)attr;
    return (osThreadId_t)1;
}

const char *osThreadGetName(osThreadId_t thread_id) {
    (void)thread_id;
    return "stub";
}

osThreadState_t osThreadGetState(osThreadId_t thread_id) {
    (void)thread_id;
    return osThreadRunning;
}

uint32_t osThreadGetStackSize(osThreadId_t thread_id) {
    (void)thread_id;
    return 2048U;
}

uint32_t osThreadGetStackSpace(osThreadId_t thread_id) {
    (void)thread_id;
    return 1024U;
}

osThreadId_t osThreadGetId(void) {
    return (osThreadId_t)1;
}

osStatus_t osThreadSetPriority(osThreadId_t thread_id, osPriority_t priority) {
    (void)thread_id;
    (void)priority;
    return osOK;
}

osPriority_t osThreadGetPriority(osThreadId_t thread_id) {
    (void)thread_id;
    return osPriorityNormal;
}

osStatus_t osThreadResume(osThreadId_t thread_id) {
    (void)thread_id;
    return osOK;
}

osStatus_t osThreadSuspend(osThreadId_t thread_id) {
    (void)thread_id;
    return osOK;
}

osStatus_t osThreadTerminate(osThreadId_t thread_id) {
    (void)thread_id;
    return osOK;
}

void osThreadExit(void) {
}

osStatus_t osThreadYield(void) {
    return osOK;
}

osStatus_t osThreadJoin(osThreadId_t thread_id) {
    (void)thread_id;
    return osErrorResource;
}

osStatus_t osThreadDetach(osThreadId_t thread_id) {
    (void)thread_id;
    return osOK;
}

osMutexId_t osMutexNew(const osMutexAttr_t *attr) {
    (void)attr;
    return (osMutexId_t)1;
}

const char *osMutexGetName(osMutexId_t mutex_id) {
    (void)mutex_id;
    return "stub";
}

osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout) {
    (void)mutex_id;
    (void)timeout;
    return osOK;
}

osStatus_t osMutexRelease(osMutexId_t mutex_id) {
    (void)mutex_id;
    return osOK;
}

osStatus_t osMutexDelete(osMutexId_t mutex_id) {
    (void)mutex_id;
    return osOK;
}

osThreadId_t osMutexGetOwner(osMutexId_t mutex_id) {
    (void)mutex_id;
    return (osThreadId_t)1;
}

osSemaphoreId_t osSemaphoreNew(uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr) {
    (void)max_count;
    (void)initial_count;
    (void)attr;
    return (osSemaphoreId_t)1;
}

const char *osSemaphoreGetName(osSemaphoreId_t semaphore_id) {
    (void)semaphore_id;
    return "stub";
}

osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphore_id, uint32_t timeout) {
    (void)semaphore_id;
    (void)timeout;
    return osOK;
}

osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphore_id) {
    (void)semaphore_id;
    return osOK;
}

uint32_t osSemaphoreGetCount(osSemaphoreId_t semaphore_id) {
    (void)semaphore_id;
    return 1U;
}

osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphore_id) {
    (void)semaphore_id;
    return osOK;
}

osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t *attr) {
    (void)attr;
    return (osEventFlagsId_t)1;
}

const char *osEventFlagsGetName(osEventFlagsId_t ef_id) {
    (void)ef_id;
    return "stub";
}

uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags) {
    (void)ef_id;
    return flags;
}

uint32_t osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags) {
    (void)ef_id;
    return flags;
}

uint32_t osEventFlagsGet(osEventFlagsId_t ef_id) {
    (void)ef_id;
    return 0U;
}

uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags, uint32_t options, uint32_t timeout) {
    (void)ef_id;
    (void)flags;
    (void)options;
    (void)timeout;
    return 0U;
}

osStatus_t osEventFlagsDelete(osEventFlagsId_t ef_id) {
    (void)ef_id;
    return osOK;
}

osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr) {
    (void)msg_count;
    (void)msg_size;
    (void)attr;
    return (osMessageQueueId_t)1;
}

const char *osMessageQueueGetName(osMessageQueueId_t mq_id) {
    (void)mq_id;
    return "stub";
}

osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout) {
    (void)mq_id;
    (void)msg_ptr;
    (void)msg_prio;
    (void)timeout;
    return osOK;
}

osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout) {
    (void)mq_id;
    (void)msg_ptr;
    (void)msg_prio;
    (void)timeout;
    return osErrorResource;
}

uint32_t osMessageQueueGetCapacity(osMessageQueueId_t mq_id) {
    (void)mq_id;
    return 16U;
}

uint32_t osMessageQueueGetMsgSize(osMessageQueueId_t mq_id) {
    (void)mq_id;
    return 4U;
}

uint32_t osMessageQueueGetCount(osMessageQueueId_t mq_id) {
    (void)mq_id;
    return 0U;
}

uint32_t osMessageQueueGetSpace(osMessageQueueId_t mq_id) {
    (void)mq_id;
    return 16U;
}

osStatus_t osMessageQueueReset(osMessageQueueId_t mq_id) {
    (void)mq_id;
    return osOK;
}

osStatus_t osMessageQueueDelete(osMessageQueueId_t mq_id) {
    (void)mq_id;
    return osOK;
}

osTimerId_t osTimerNew(osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr) {
    (void)func;
    (void)type;
    (void)argument;
    (void)attr;
    return (osTimerId_t)1;
}

const char *osTimerGetName(osTimerId_t timer_id) {
    (void)timer_id;
    return "stub";
}

osStatus_t osTimerStart(osTimerId_t timer_id, uint32_t ticks) {
    (void)timer_id;
    (void)ticks;
    return osOK;
}

osStatus_t osTimerStop(osTimerId_t timer_id) {
    (void)timer_id;
    return osOK;
}

uint32_t osTimerGetTimerId(osTimerId_t timer_id) {
    (void)timer_id;
    return 0U;
}

osStatus_t osTimerDelete(osTimerId_t timer_id) {
    (void)timer_id;
    return osOK;
}

int32_t osKernelLock(void) {
    return 0;
}

int32_t osKernelUnlock(void) {
    return 0;
}

int32_t osKernelRestoreLock(int32_t lock) {
    (void)lock;
    return 0;
}

int32_t osKernelLockTickless(uint32_t ticks) {
    (void)ticks;
    return 0;
}

int32_t osKernelUnlockTickless(uint32_t ticks) {
    (void)ticks;
    return 0;
}

int32_t osKernelRestoreLockTickless(int32_t lock, uint32_t ticks) {
    (void)lock;
    (void)ticks;
    return 0;
}

uint32_t osKernelGetSysTimerCount(void) {
    return HAL_GetTick() * (SystemCoreClock / 1000U);
}

uint32_t osKernelGetSysTimerFreq(void) {
    return SystemCoreClock;
}
