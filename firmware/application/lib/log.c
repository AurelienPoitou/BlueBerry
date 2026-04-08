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
static void log_format_and_write(const char *prefix,
                                 const char *file,
                                 int line,
                                 const char *msg)
{
    bool is_binary = false;
    size_t msg_len = strnlen(msg, LOG_MESSAGE_SIZE);

    for (size_t i = 0; i < msg_len; i++) {
        unsigned char c = msg[i];

        // Allow ASCII printable + newline + tab
        if ((c >= 0x20 && c <= 0x7E) || c == '\n' || c == '\t')
            continue;

        // Allow UTF‑8 continuation bytes (0x80–0xBF)
        if (c >= 0x80 && c <= 0xBF)
            continue;

        // Allow UTF‑8 leading bytes (0xC0–0xF4)
        if (c >= 0xC0 && c <= 0xF4)
            continue;

        // Everything else is binary
        is_binary = true;
        break;
    }

    char output[LOG_MESSAGE_SIZE];
    long long unsigned ts = (long long unsigned) TimerGetMillis();

    if (is_binary) {
        // Hex dump buffer
        char hex[LOG_MESSAGE_SIZE * 3];
        size_t p = 0;
        for (size_t i = 0; i < msg_len && p < sizeof(hex) - 4; i++) {
            p += snprintf(hex + p, sizeof(hex) - p, "%02X ", (unsigned char)msg[i]);
        }

        // Print diagnostic message
        snprintf(output, sizeof(output),
                 "[%llu] BINARY LOG DETECTED from %s\n"
                 "  Raw length: %zu bytes\n"
                 "  Hex dump: %s\n",
                 ts, prefix ? prefix : "(no prefix)", msg_len, hex);

        log_write(output);
        return;
    }

    snprintf(output, sizeof(output),
             "[%llu] %s (%s:%d): %s\n",
             ts,
             prefix ? prefix : "",
             file,
             line,
             msg);

    log_write(output);
}

/* ---------------- PUBLIC API (unchanged signatures) ---------------- */

void LogDebugInternal(uint8_t src, const char *file, int line,
                      const char *fmt, ...)
{
    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    log_format_and_write("DEBUG", file, line, buffer);
}

void LogInfoInternal(uint8_t src, const char *file, int line,
                     const char *fmt, ...)
{
    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    log_format_and_write("INFO", file, line, buffer);
}

void LogErrorInternal(const char *file, int line,
                      const char *fmt, ...)
{
    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    log_format_and_write("ERROR", file, line, buffer);
}

void LogWarningInternal(const char *file, int line,
                        const char *fmt, ...)
{
    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    log_format_and_write("WARN", file, line, buffer);
}

void LogRawInternal(const char *file, int line,
                         const char *fmt, ...)
{
    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // RAWDEBUG must NOT add prefix, file, line, or newline.
    // It must behave exactly like the old logger.
    pthread_mutex_lock(&log_mutex);
    fputs(buffer, stdout);   // no newline
    fflush(stdout);
    pthread_mutex_unlock(&log_mutex);
}

void LogMessageInternal(const char *file, int line,
                        const char *prefix, const char *msg)
{
    log_format_and_write(prefix, file, line, msg);
}
