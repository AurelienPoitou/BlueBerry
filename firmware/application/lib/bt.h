#include <stdint.h>
#ifndef BT_H
#define BT_H
#include <stdint.h>
#include "../mappings.h"
#include "bt/bt_common.h"
#include "bt/bt_rpi4.h"

typedef struct BTProcessArgs {
    BT_t *bt;
} BTProcessArgs;

BT_t BTInit();
void BTCommandCallAccept(BT_t *);
void BTCommandCallEnd(BT_t *);
void BTCommandDial(BT_t *, const char *, const char *);
void BTCommandRedial(BT_t *);
void BTCommandDisconnect(BT_t *);
void BTCommandConnect(BT_t *, BTPairedDevice_t *);
void BTCommandGetMetadata(BT_t *);
void BTCommandList(BT_t *);
void BTCommandPause(BT_t *);
void BTCommandPlay(BT_t *);
void BTCommandPlaybackTrackFastforwardStart(BT_t *);
void BTCommandPlaybackTrackFastforwardStop(BT_t *);
void BTCommandPlaybackTrackRewindStart(BT_t *);
void BTCommandPlaybackTrackRewindStop(BT_t *);
void BTCommandPlaybackTrackNext(BT_t *);
void BTCommandPlaybackTrackPrevious(BT_t *);
void BTCommandProfileOpen(BT_t *);
void BTCommandSetConnectable(BT_t *, unsigned char);
void BTCommandSetDiscoverable(BT_t *, unsigned char);
void BTCommandToggleVoiceRecognition(BT_t *);
uint8_t BTHasActiveMacId(BT_t *);
void *BTProcess(void *);
#endif /* BT_H */
