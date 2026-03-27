#include <stdio.h>
#include <stdint.h>
#include "log.h"

void LogMessage(const char *type, const char *data)
{
    //UART_t *debugger = UARTGetModuleHandler(SYSTEM_UART_MODULE);
    if (1) {
        char output[LOG_MESSAGE_SIZE] = {0};
        long long unsigned int ts = (long long unsigned int) TimerGetMillis();
        snprintf(output, LOG_MESSAGE_SIZE - 1 , "[%llu] %s: %s\r\n", ts, type, data);
        printf(output);
        //UARTSendString(debugger, output);
    }
}

void LogRaw(const char *format, ...)
{
    //UART_t *debugger = UARTGetModuleHandler(SYSTEM_UART_MODULE);
    if (1) {
        char buffer[LOG_MESSAGE_SIZE] = {0};
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
        va_end(args);
        printf(buffer);
        //UARTSendString(debugger, buffer);
    }
}

void LogRawDebug(uint8_t source, const char *format, ...)
{
    //UART_t *debugger = UARTGetModuleHandler(SYSTEM_UART_MODULE);
        char buffer[LOG_MESSAGE_SIZE] = {0};
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
        va_end(args);
        printf(buffer);
}

void LogDebug(uint8_t source, const char *format, ...)
{
        char buffer[LOG_MESSAGE_SIZE] = {0};
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
        va_end(args);
        LogMessage("DEBUG", buffer);
}

void LogError(const char *format, ...)
{
    char buffer[LOG_MESSAGE_SIZE] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
    va_end(args);
    LogMessage("ERROR", buffer);
}

void LogInfo(uint8_t source, const char *format, ...)
{
        char buffer[LOG_MESSAGE_SIZE] = {0};
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
        va_end(args);
        LogMessage("INFO", buffer);
}

void LogWarning(const char *format, ...)
{
    char buffer[LOG_MESSAGE_SIZE] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
    va_end(args);
    LogMessage("WARNING", buffer);
}
