#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>
#include "log.h"

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Internal helper: thread-safe printf + flush */
static void log_write(const char *msg)
{
    pthread_mutex_lock(&log_mutex);
    fputs(msg, stdout);
    fflush(stdout);          // CRITICAL for systemd services
    pthread_mutex_unlock(&log_mutex);
}

/* Internal helper: format + write */
static void log_format_and_write(const char *prefix, const char *msg)
{
    char output[LOG_MESSAGE_SIZE];
    long long unsigned ts = (long long unsigned) TimerGetMillis();

    if (prefix)
        snprintf(output, sizeof(output), "[%llu] %s: %s\n", ts, prefix, msg);
    else
        snprintf(output, sizeof(output), "[%llu] %s\n", ts, msg);

    log_write(output);
}

/* ---------------- PUBLIC API (unchanged signatures) ---------------- */

void LogMessage(const char *type, const char *data)
{
    log_format_and_write(type, data);
}

void LogRaw(const char *format, ...)
{
    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    log_write(buffer);
}

void LogRawDebug(uint8_t source, const char *format, ...)
{
    (void)source; // unused for now

    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    log_write(buffer);
}

void LogDebug(uint8_t source, const char *format, ...)
{
    (void)source;

    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    LogMessage("DEBUG", buffer);
}

void LogError(const char *format, ...)
{
    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    LogMessage("ERROR", buffer);
}

void LogInfo(uint8_t source, const char *format, ...)
{
    (void)source;

    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    LogMessage("INFO", buffer);
}

void LogWarning(const char *format, ...)
{
    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    LogMessage("WARNING", buffer);
}
