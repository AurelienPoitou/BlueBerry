/*
 * File: timer.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a timer that fires every millisecond so that we can
 *     time events in the application. Implement a scheduled task queue.
 */

#include "timer.h"
#include <unistd.h> // For usleep
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>   // For clock_gettime
#include "log.h"

#define TIMER_TASKS_MAX 32 // Define the maximum number of scheduled tasks

volatile TimerScheduledTask_t TimerRegisteredTasks[TIMER_TASKS_MAX];
uint8_t TimerRegisteredTasksCount = 0;

// Function to initialize the timer
void TimerInit() {
    TimerRegisteredTasksCount = 0;
    LogDebug(LOG_SOURCE_SYSTEM, "Timer initialized.");
}

// Function to get the current milliseconds
uint32_t TimerGetMillis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// Function to delay in milliseconds
void TimerDelayMilliseconds(uint16_t delay) {
    usleep(delay * 1000); // Convert milliseconds to microseconds
}

// Function to delay in milliseconds
void TimerDelayMicroseconds(uint16_t delay) {
    usleep(delay);
}

// Function to process scheduled tasks
void TimerProcessScheduledTasks() {
    uint32_t currentMillis = TimerGetMillis(); // Get current time in milliseconds
    for (uint8_t idx = 0; idx < TimerRegisteredTasksCount; idx++) {
        volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[idx];
//        LogDebug(LOG_SOURCE_SYSTEM, "Checking TimerScheduledTask_t instance: { task: %p, context: %p, interval: %u, next_run: %d }\n",
//            (void *)t->task, t->context, t->interval, t->next_execution);
        if (t->task != NULL && currentMillis >= t->next_execution) {
            t->task(t->context); // Execute the scheduled task
//            LogDebug(LOG_SOURCE_SYSTEM, "Executing TimerScheduledTask_t instance: { task: %p, context: %p }\n", (void *)t->task, t->context);
            t->next_execution = currentMillis + t->interval; // Schedule next execution
        }
    }
}

// Function to register a scheduled task
uint8_t TimerRegisterScheduledTask(TimerTask_t task, void *ctx, uint16_t interval) {
    if (TimerRegisteredTasksCount >= TIMER_TASKS_MAX) {
        LogError("FAILED TO REGISTER TIMER -- Allocations Full");
        return 0;
    }
    if (interval == TIMER_TASK_DISABLED) {
        return 0;
    }
    TimerScheduledTask_t scheduledTask;
    scheduledTask.task = task;
    scheduledTask.context = ctx;
    scheduledTask.interval = interval;
    scheduledTask.next_execution = TimerGetMillis() + interval; // Set the next execution time
    LogDebug(LOG_SOURCE_SYSTEM, "Registering TimerScheduledTask_t instance: { task: %p, context: %p, interval: %u, next_run: %d }\n",
             (void *)task, ctx, interval, scheduledTask.next_execution);
    TimerRegisteredTasks[TimerRegisteredTasksCount++] = scheduledTask;
    return TimerRegisteredTasksCount - 1; // Return the task ID
}

// Function to unregister a scheduled task
uint8_t TimerUnregisterScheduledTask(TimerTask_t task) {
    for (uint8_t idx = 0; idx < TimerRegisteredTasksCount; idx++) {
        volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[idx];
        if (t->task == task) {
            memset((void *)t, 0, sizeof(TimerScheduledTask_t)); // Clear the task
            LogDebug(LOG_SOURCE_SYSTEM, "Unregistered TimerScheduledTask_t instance: { task: %p }\n", (void *)task);
            return 0; // Success
        }
    }
    return 1; // Task not found
}

// Function to reset a scheduled task
void TimerResetScheduledTask(uint8_t taskId) {
    if (taskId < TimerRegisteredTasksCount) {
        volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[taskId];
        if (t->task != NULL) {
            LogDebug(LOG_SOURCE_SYSTEM, "Resetting TimerScheduledTask_t instance: { task: %p }\n", (void *)t->task);
            t->next_execution = TimerGetMillis() + t->interval; // Reset the next execution time
        }
    }
}

// Function to set the task interval
void TimerSetTaskInterval(uint8_t taskId, uint16_t interval) {
    if (taskId < TimerRegisteredTasksCount) {
        volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[taskId];
        if (t->task != NULL) {
            t->interval = interval; // Update the interval
            t->next_execution = TimerGetMillis() + interval; // Reset the next execution time
            LogDebug(LOG_SOURCE_SYSTEM, "Updated TimerScheduledTask_t instance interval: { task: %p, new interval: %u }\n",
                      (void *)t->task, interval);
        }
    }
}

// Function to trigger a scheduled task
void TimerTriggerScheduledTask(uint8_t taskId) {
    if (taskId < TimerRegisteredTasksCount) {
        volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[taskId];
        if (t->task != NULL) {
            LogDebug(LOG_SOURCE_SYSTEM, "Triggering TimerScheduledTask_t instance: { task: %p }\n", (void *)t->task);
            t->task(t->context); // Execute the task
            t->next_execution = TimerGetMillis() + t->interval; // Schedule next execution
        }
    }
}
