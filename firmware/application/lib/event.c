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
    LogDebug(LOG_SOURCE_SYSTEM, "Triggering event: %d - %s", eventType, data);
    for (idx = 0; idx < EVENT_CALLBACKS_COUNT; idx++) {
        volatile Event_t *cb = &EVENT_CALLBACKS[idx];
        if (cb->type == eventType) {
            cb->callback(cb->context, data);
        }
    }
}
