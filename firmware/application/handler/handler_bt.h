#include <stdint.h>
#ifndef HANDLER_BC127_H
#define HANDLER_BC127_H
#include <stdio.h>
#include "../lib/bt/bt_common.h"
#include "../lib/bt/bt_rpi4.h"
#include "../lib/log.h"
#include "../lib/event.h"
#include "../lib/ibus.h"
#include "../lib/timer.h"
#include "../lib/utils.h"
#include "../ui/bmbt.h"
#include "../ui/cd53.h"
#include "../ui/mid.h"
#include "handler_common.h"

void HandlerBTInit(HandlerContext_t *);
void HandlerBTCallStatus(void *, uint8_t *);
void HandlerBTDeviceFound(void *, uint8_t *);
void HandlerBTCallerID(void *, uint8_t *);
void HandlerBTDeviceLinkConnected(void *, uint8_t *);
void HandlerBTDeviceDisconnected(void *, uint8_t *);
void HandlerBTPlaybackStatus(void *, uint8_t *);
void HandlerBTTimeUpdate(void *, uint8_t *);
void HandlerUICloseConnection(void *, uint8_t *);
void HandlerUIInitiateConnection(void *, uint8_t *);

/*void HandlerBTBC127Boot(void *, uint8_t *);
void HandlerBTBC127BootStatus(void *, uint8_t *);

void HandlerBTBM83AVRCPUpdates(void *, uint8_t *);
void HandlerBTBM83Boot(void *, uint8_t *);
void HandlerBTBM83BootStatus(void *, uint8_t *);
void HandlerBTBM83DSPStatus(void *, uint8_t *);
*/
void HandlerTimerBTTCUStateChange(void *);
void HandlerTimerBTVolumeManagement(void *);

void HandlerBTRPI4Boot(void *, uint8_t *);
void HandlerBTRPI4BootStatus(void *, uint8_t *);

void HandlerTimerBTRPI4State(void *);
void HandlerTimerBTRPI4DeviceConnection(void *);
void HandlerTimerBTRPI4ScanDevices(void *);
void HandlerTimerBTRPI4Metadata(void *);

#endif /* HANDLER_BC127_H */
