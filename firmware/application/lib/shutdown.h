#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#include <signal.h>
#include <stdatomic.h>

extern volatile sig_atomic_t shutting_down;

void install_shutdown_handlers(void);

#endif

