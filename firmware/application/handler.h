#include <stdint.h>
#ifndef HANDLER_H
#define HANDLER_H
#include "handler/handler_bt.h"
#include "handler/handler_common.h"
#include "lib/bt/bt_rpi4.h"
#include "handler/handler_ibus.h"
#include "lib/bt.h"
#include "lib/log.h"
#include "lib/event.h"
#include "lib/ibus.h"
#include "lib/timer.h"
#include "lib/utils.h"

void HandlerInit(BT_t *, IBus_t *);
void HandlerUICloseConnection(void *, unsigned char *);
void HandlerUIInitiateConnection(void *, unsigned char *);
void HandlerTimerPoweroff(void *);
#endif /* HANDLER_H */
