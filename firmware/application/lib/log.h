#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "../mappings.h"
#include "config.h"
#include "timer.h"

#define LOG_MESSAGE_SIZE 416

#define LOG_SOURCE_BT     CONFIG_DEVICE_LOG_BT
#define LOG_SOURCE_IBUS   CONFIG_DEVICE_LOG_IBUS
#define LOG_SOURCE_SYSTEM CONFIG_DEVICE_LOG_SYSTEM
#define LOG_SOURCE_UI     CONFIG_DEVICE_LOG_UI
#define LOG_SOURCE_CONFIG CONFIG_DEVICE_LOG_CONFIG

//
// PUBLIC API (unchanged for callers)
// These macros inject __FILE__ and __LINE__
//
#define LogDebug(src, fmt, ...) \
    LogDebugInternal(src, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LogInfo(src, fmt, ...) \
    LogInfoInternal(src, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LogError(fmt, ...) \
    LogErrorInternal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LogWarning(fmt, ...) \
    LogWarningInternal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LogRaw(fmt, ...) \
    LogRawInternal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LogMessage(prefix, msg) \
    LogMessageInternal(__FILE__, __LINE__, prefix, msg)

//
// INTERNAL IMPLEMENTATION (defined in log.c)
//
void LogDebugInternal(uint8_t src, const char *file, int line,
                      const char *fmt, ...);

void LogInfoInternal(uint8_t src, const char *file, int line,
                     const char *fmt, ...);

void LogErrorInternal(const char *file, int line,
                      const char *fmt, ...);

void LogWarningInternal(const char *file, int line,
                        const char *fmt, ...);

void LogRawInternal(const char *file, int line,
                    const char *fmt, ...);

void LogMessageInternal(const char *file, int line,
                        const char *prefix, const char *msg);

#endif /* LOG_H */
