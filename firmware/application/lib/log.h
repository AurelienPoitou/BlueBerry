#include <stdint.h>
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
#define LOG_SOURCE_BT CONFIG_DEVICE_LOG_BT
#define LOG_SOURCE_IBUS CONFIG_DEVICE_LOG_IBUS
#define LOG_SOURCE_SYSTEM CONFIG_DEVICE_LOG_SYSTEM
#define LOG_SOURCE_UI CONFIG_DEVICE_LOG_UI
#define LOG_SOURCE_CONFIG CONFIG_DEVICE_LOG_CONFIG

void LogMessage(const char *, const char *);
void LogRaw(const char *, ...);
void LogRawDebug(uint8_t, const char *, ...);
void LogError(const char *, ...);
void LogDebug(uint8_t, const char *, ...);
void LogInfo(uint8_t, const char *, ...);
void LogWarning(const char *, ...);
#endif /* LOG_H */
