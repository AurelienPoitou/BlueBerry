#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define TIMER_TASKS_MAX 32
#define TIMER_INDEX 0
#define TIMER_TASK_DISABLED 0

typedef void (*TimerTask_t)(void *context);

typedef struct TimerScheduledTask_t {
    TimerTask_t task;
    void *context;
    uint16_t interval;
    uint32_t next_execution;
} TimerScheduledTask_t;

void TimerInit();
uint32_t TimerGetMillis();
void TimerDelayMilliseconds(uint16_t delay);
void TimerDelayMicroseconds(uint16_t);
uint8_t TimerRegisterScheduledTask(TimerTask_t task, void *ctx, uint16_t interval);
uint8_t TimerUnregisterScheduledTask(TimerTask_t task);
void TimerResetScheduledTask(uint8_t taskId);
void TimerSetTaskInterval(uint8_t taskId, uint16_t interval);
void TimerTriggerScheduledTask(uint8_t taskId);
void TimerProcessScheduledTasks();

#endif // TIMER_H
