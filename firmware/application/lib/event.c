#include <stdint.h>
#include "event.h"
#include "log.h"
volatile Event_t EVENT_CALLBACKS[EVENT_MAX_CALLBACKS];
uint8_t EVENT_CALLBACKS_COUNT = 0;

void EventRegisterCallback(uint8_t eventType, void *callback, void *context)
{
    Event_t cb;
    cb.type = eventType;
    LogDebug(LOG_SOURCE_SYSTEM, "Registering event callback for: %d", eventType);
    cb.callback = callback;
    cb.context = context;
    EVENT_CALLBACKS[EVENT_CALLBACKS_COUNT++] = cb;
}

uint8_t EventUnregisterCallback(uint8_t eventType, void *callback)
{
    uint8_t idx;
    for (idx = 0; idx < EVENT_CALLBACKS_COUNT; idx++) {
        volatile Event_t *cb = &EVENT_CALLBACKS[idx];
        if (cb->type == eventType &&
            cb->callback == callback
        ) {
            memset((void *) cb, 0, sizeof(Event_t));
            return 0;
        }
    }
    return 1;
}

void EventTriggerCallback(uint8_t eventType, unsigned char *data)
{
    uint8_t idx;
    if (data == NULL) {
        LogDebug(LOG_SOURCE_SYSTEM, "Triggering event: %d - <null>", eventType);
    } else {
        // Detect if data looks like a string
        bool looks_string = true;
        for (size_t i = 0; i < 64; i++) {
            unsigned char c = data[i];
            if (c == 0) break; // null-terminated → OK
            if (c < 0x20 || c > 0x7E) { // control or non-ASCII
                looks_string = false;
                break;
            }
        }

        if (looks_string) {
            LogDebug(LOG_SOURCE_SYSTEM, "Triggering event: %d - %s", eventType, data);
        } else {
            char hex[256];
            size_t p = 0;
            for (size_t i = 0; i < 32; i++) {   // dump up to 32 bytes
                p += snprintf(hex + p, sizeof(hex) - p, "%02X ", data[i]);
            }
            LogDebug(LOG_SOURCE_SYSTEM, "Triggering event: %d - <binary>: %s", eventType, hex);
        }
    }

    for (idx = 0; idx < EVENT_CALLBACKS_COUNT; idx++) {
        volatile Event_t *cb = &EVENT_CALLBACKS[idx];
        if (cb->type == eventType) {
            cb->callback(cb->context, data);
        }
    }
}
