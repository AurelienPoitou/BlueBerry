#ifndef RPI4_H
#define RPI4_H
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../../mappings.h"
#include "../log.h"
#include "../event.h"
#include "../timer.h"
#include "../utils.h"
#include "bt_common.h"

#define RPI4_AUDIO_I2S "0"
#define RPI4_AUDIO_SPDIF "2"
#define RPI4_CLOSE_ALL 255
#define RPI4_DEVICE_NAME_LEN 64
#define RPI4_DEVICE_NAME_OFFSET 19
#define RPI4_MAX_DEVICE_PAIRED 8
#define RPI4_MAX_DEVICE_PROFILES 5
#define RPI4_METADATA_TITLE_OFFSET 22
#define RPI4_METADATA_ARTIST_OFFSET 23
#define RPI4_METADATA_ALBUM_OFFSET 22
#define RPI4_MSG_END_CHAR 0x0D
#define RPI4_MSG_LF_CHAR 0x0A
#define RPI4_MSG_DELIMETER 0x20
#define RPI4_SHORT_NAME_MAX_LEN 8
#define RPI4_PROFILE_COUNT 9
#define RPI4_RX_QUEUE_TIMEOUT 750
#define RPI4_LINK_A2DP 0
#define RPI4_LINK_AVRCP 1
#define RPI4_LINK_HFP 3
#define RPI4_LINK_BLE 4
#define RPI4_LINK_MAP 8
#define RPI4_AT_DATE_YEAR 0
#define RPI4_AT_DATE_MONTH 1
#define RPI4_AT_DATE_DAY 2
#define RPI4_AT_DATE_HOUR 3
#define RPI4_AT_DATE_MIN 4
#define RPI4_AT_DATE_SEC 5

void RPI4ClearActiveDevice(BT_t *);
void RPI4ClearConnections(BT_t *);
void BTClearMetadata(BT_t *);
void RPI4ClearPairedDevices(BT_t *);
void RPI4ClearInactivePairedDevices(BT_t *);
void RPI4ClearPairingErrors(BT_t *);
void RPI4CommandAT(BT_t *, char *);
void RPI4CommandATSet(BT_t *, char *, char *);
void RPI4CommandBackward(BT_t *);
void RPI4CommandBackwardSeekPress(BT_t *);
void RPI4CommandBackwardSeekRelease(BT_t *);
void RPI4CommandBtState(BT_t *, uint8_t, uint8_t);
void RPI4CommandCallAnswer(BT_t *);
void RPI4CommandCallEnd(BT_t *);
void RPI4CommandCallReject(BT_t *);
void RPI4CommandDisconnect(BT_t *);
void RPI4CommandCVC(BT_t *, char *, uint8_t, uint8_t);
void RPI4CommandCVCParams(BT_t *, char *);
void RPI4CommandForward(BT_t *);
void RPI4CommandForwardSeekPress(BT_t *);
void RPI4CommandForwardSeekRelease(BT_t *);
void RPI4CommandGetDeviceName(BT_t *, char *);
void RPI4CommandGetMetadata(BT_t *);
void RPI4CommandLicense(BT_t *, char *, char *);
void RPI4CommandList(BT_t *);
void RPI4CommandPause(BT_t *);
void RPI4CommandPlay(BT_t *);
void RPI4CommandProfileClose(BT_t *, uint8_t);
void RPI4CommandConnect(BT_t *, BTPairedDevice_t *);
void RPI4CommandReset(BT_t *);
void RPI4CommandSetAudio(BT_t *, uint8_t, uint8_t);
void RPI4CommandSetAudioAnalog(BT_t *, uint8_t, uint8_t, uint8_t, char *);
void RPI4CommandSetAudioDigital(BT_t *, char *,char *, char *, char *);
void RPI4CommandSetAutoConnect(BT_t *, uint8_t);
void RPI4CommandSetBtState(BT_t *, uint8_t, uint8_t);
void RPI4CommandSetBtVolConfig(BT_t *, uint8_t, uint8_t, uint8_t, uint8_t);
void RPI4CommandSetCOD(BT_t *, uint32_t);
void RPI4CommandSetCodec(BT_t *, uint8_t, char *);
void RPI4CommandSetMetadata(BT_t *, uint8_t);
void RPI4CommandSetMicGain(BT_t *, unsigned char, unsigned char, unsigned char);
void RPI4CommandSetModuleName(BT_t *, char *);
void RPI4CommandSetPin(BT_t *, char *);
void RPI4CommandSetProfiles(BT_t *, uint8_t, uint8_t, uint8_t, uint8_t);
void RPI4CommandSetUART(BT_t *, uint32_t, char *, uint8_t);
void RPI4CommandStatus(BT_t *);
void RPI4CommandStatusAVRCP(BT_t *);
void RPI4CommandToggleVR(BT_t *);
void RPI4CommandTone(BT_t *, char *);
void RPI4CommandUnpair(BT_t *);
void RPI4CommandVersion(BT_t *);
void RPI4CommandVolume(BT_t *, uint8_t, char *);
void RPI4CommandWrite(BT_t *);
uint8_t RPI4GetConnectedDeviceCount(BT_t *);
uint8_t RPI4GetDeviceId(char *);
void RPI4ProcessEventA2DPStreamSuspend(BT_t *, char **);
void RPI4ProcessEventAbsVol(BT_t *, char **);
void RPI4ProcessEventAT(BT_t *, char **, uint8_t);
void RPI4ProcessEventAVRCPMedia(BT_t *, char **, char *);
void RPI4ProcessEventAVRCPPlay(BT_t *, char **);
void RPI4ProcessEventAVRCPPause(BT_t *, char **);
void RPI4ProcessEventAVRCPPause(BT_t *, char **);
void RPI4ProcessEventBuild(BT_t *, char **);
void RPI4ProcessEventCall(BT_t *, uint8_t);
void RPI4ProcessEventCloseOk(BT_t *, char **);
void RPI4ProcessEventLink(BT_t *, char **);
void RPI4ProcessEventList(BT_t *, char **);
void RPI4ProcessEventName(BT_t *, char **, char *);
void RPI4ProcessEventOpenError(BT_t *, char **);
void RPI4ProcessEventOpenOk(BT_t *, char **);
void RPI4ProcessEventSCO(BT_t *, uint8_t);
void RPI4ProcessEventState(BT_t *, char **);
void RPI4Process(BT_t *);
void RPI4SendCommand(BT_t *, char *);
void RPI4SendCommandEmpty(BT_t *);

#endif /* RPI4 */
