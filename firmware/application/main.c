#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <glib.h>

#include "handler.h"
#include "mappings.h"
#include "lib/bt.h"
#include "lib/config.h"
#include "lib/log.h"
#include "lib/ibus.h"
#include "lib/timer.h"
#include "lib/utils.h"
#include "shutdown.h"   // contains shutting_down + install_shutdown_handlers()

/* Global GLib loop owned by main thread */
GMainLoop *main_loop = NULL;

/* Worker thread IDs */
static pthread_t bt_thread_id;
static pthread_t ibus_thread_id;

/* Timer callback integrated into GLib */
static gboolean on_timer_tick(gpointer user_data)
{
    (void)user_data;

    if (shutting_down)
        return G_SOURCE_REMOVE;

    TimerProcessScheduledTasks();
    return G_SOURCE_CONTINUE;
}

/* Worker thread entry points */
void *BTProcess(void *arg);
void *IBusProcess(void *arg);

int main(void)
{
    LogInfo(LOG_SOURCE_SYSTEM, "Starting Application");

    /* Install SIGINT + SIGTERM handlers */
    install_shutdown_handlers();

    /* Init low-level modules */
    TimerInit();

    /* Optional: IBUS_PORT from environment */
    char *env = getenv("IBUS_PORT");
    LogInfo(LOG_SOURCE_SYSTEM, "IBUS_PORT:%s", env);
    if (env)
        portname = env;

    /* Init BT + IBus */
    struct BT_t   bt   = BTInit();
    struct IBus_t ibus = IBusInit();

    /* Init handlers */
    HandlerInit(&bt, &ibus);

    /* Prepare thread args */
    BTProcessArgs *btArgs = malloc(sizeof(BTProcessArgs));
    btArgs->bt = &bt;

    IBusProcessArgs *ibusArgs = malloc(sizeof(IBusProcessArgs));
    ibusArgs->ibus = &ibus;

    /* Start worker threads (NO GLib loops inside them) */
    if (pthread_create(&bt_thread_id, NULL, BTProcess, btArgs) != 0) {
        LogError("Failed to create BTProcess thread");
        free(btArgs);
        free(ibusArgs);
        return EXIT_FAILURE;
    }

    if (pthread_create(&ibus_thread_id, NULL, IBusProcess, ibusArgs) != 0) {
        LogError("Failed to create IBusProcess thread");
        shutting_down = 1;
        pthread_join(bt_thread_id, NULL);
        free(btArgs);
        free(ibusArgs);
        return EXIT_FAILURE;
    }

    /* Startup banner */
    long long unsigned int ts = (long long unsigned int) TimerGetMillis();
    LogRaw("\r\n");
    LogRaw("[%llu] ═══════════════════════════════════════════════════════════\r\n", ts);
    LogRaw("[%llu] IBus System Initialization Complete\r\n", ts);
    LogRaw("[%llu] ═══════════════════════════════════════════════════════════\r\n", ts);
    LogRaw("[%llu] Baud Rate: 9600 (8-N-2, ODD Parity)\r\n", ts);
    LogRaw("[%llu] RX Buffer: Ready (%u bytes)\r\n", ts, IBUS_RX_BUFFER_SIZE);
    LogRaw("[%llu] TX Buffer: Ready (%u bytes)\r\n", ts, IBUS_TX_BUFFER_SIZE);
    LogRaw("[%llu] Status: Waiting for I-Bus traffic\r\n", ts);
    LogRaw("[%llu] ═══════════════════════════════════════════════════════════\r\n", ts);
    LogRaw("[%llu] Entering main loop (GLib)...\r\n", ts);
    LogRaw("[%llu] ═══════════════════════════════════════════════════════════\r\n\r\n", ts);

    /* Create GLib main loop */
    main_loop = g_main_loop_new(NULL, FALSE);

    /* Integrate TimerProcessScheduledTasks into GLib */
    g_timeout_add(10, on_timer_tick, NULL);  // 10 ms

    /* Run until SIGTERM/SIGINT triggers g_main_loop_quit() */
    g_main_loop_run(main_loop);

    LogInfo(LOG_SOURCE_SYSTEM, "Main GLib loop exited, shutting down");

    /* Signal worker threads to stop */
    shutting_down = 1;

    /* Join worker threads */
    pthread_join(bt_thread_id, NULL);
    pthread_join(ibus_thread_id, NULL);

    /* Cleanup */
    free(btArgs);
    free(ibusArgs);

    g_main_loop_unref(main_loop);
    main_loop = NULL;

    LogInfo(LOG_SOURCE_SYSTEM, "Application terminated cleanly");
    return 0;
}

