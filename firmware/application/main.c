#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pigpio.h>
//#include "sysconfig.h"
#include <pthread.h>
#include "handler.h"
#include "mappings.h"
#include "lib/bt.h"
#include "lib/config.h"
#include "lib/log.h"
#include "lib/ibus.h"
#include "lib/timer.h"
#include "lib/utils.h"

#define ON_LED_PIN 0 // Define the GPIO pin for ON_LED

int main(void)
{
    printf("Starting..");
    LogInfo(LOG_SOURCE_SYSTEM, "Starting Application");
    if (gpioInitialise() < 0) {
        LogError("pigpio initialisation failed\n");
        return 1; // Exit if initialization fails
    }

    // Initialize low level modules
    TimerInit();

    // Initialize Bluetooth and other components
    struct BT_t bt = BTInit();
    struct IBus_t ibus = IBusInit();

    // Initialize handlers
    HandlerInit(&bt, &ibus);

    BTProcessArgs *btArgs = malloc(sizeof(BTProcessArgs));
    btArgs->bt = &bt;

    // Create a thread to run BTProcess
    pthread_t bt_thread_id;
    if (pthread_create(&bt_thread_id, NULL, BTProcess, btArgs) != 0) {
        LogError("Failed to create BTProcess thread\n");
        free(btArgs);
        return EXIT_FAILURE;
    }

    IBusProcessArgs *ibusArgs = malloc(sizeof(IBusProcessArgs));
    ibusArgs->ibus = &ibus;

    // Create a thread to run IBusProcess
    pthread_t ibus_thread_id;
    if (pthread_create(&ibus_thread_id, NULL, IBusProcess, ibusArgs) != 0) {
        LogError("Failed to create IBusProcess thread\n");
        free(ibusArgs);
        return EXIT_FAILURE;
    }

    // Main loop
    while (1) {
//        BTProcess(&bt);
//        IBusProcess(&ibus);
        TimerProcessScheduledTasks();
        // Add any other processing needed
    }

    gpioTerminate();
    return 0;
}

// Function to wait and turn off the LED
void TrapWait()
{
    gpioWrite(ON_LED_PIN, 0); // Turn off the LED (LOW)
    gpioDelay(50); // Wait for 50 milliseconds
    gpioWrite(ON_LED_PIN, 1); // Turn on the LED (HIGH)
}

// Remove interrupt handlers as they are not applicable for Raspberry Pi
