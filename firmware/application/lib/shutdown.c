#include "shutdown.h"
#include <glib.h>

volatile sig_atomic_t shutting_down = 0;

/* This pointer is set by main.c after creating the loop */
extern GMainLoop *main_loop;

static void handle_signal(int signo)
{
    shutting_down = 1;

    /* Wake + stop the main GLib loop */
    if (main_loop != NULL)
        g_main_loop_quit(main_loop);
}

void install_shutdown_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
}
