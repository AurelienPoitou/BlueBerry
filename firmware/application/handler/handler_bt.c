#include <stdint.h>
//#include "../lib/utils.h"
#include "handler_bt.h"
#include "handler_common.h"
static char *PROFILES[] = {
    "A2DP",
    "AVRCP",
    0,
    "HFP",
    "BLE",
    0,
    "PBAP",
    0,
    "MAP"
};

void HandlerBTInit(HandlerContext_t *context)
{
    LogDebug(LOG_SOURCE_BT, "HandlerBTInit");
    EventRegisterCallback(
        BT_EVENT_CALL_STATUS_UPDATE,
        &HandlerBTCallStatus,
        context
    );
    EventRegisterCallback(
        BT_EVENT_CALLER_ID_UPDATE,
        &HandlerBTCallerID,
        context
    );
    EventRegisterCallback(
        BT_EVENT_TIME_UPDATE,
        &HandlerBTTimeUpdate,
        context
    );
    EventRegisterCallback(
        BT_EVENT_DEVICE_FOUND,
        &HandlerBTDeviceFound,
        context
    );
    EventRegisterCallback(
        BT_EVENT_DEVICE_LINK_CONNECTED,
        &HandlerBTDeviceLinkConnected,
        context
    );
    EventRegisterCallback(
        BT_EVENT_DEVICE_LINK_DISCONNECTED,
        &HandlerBTDeviceDisconnected,
        context
    );
    EventRegisterCallback(
        BT_EVENT_PLAYBACK_STATUS_CHANGE,
        &HandlerBTPlaybackStatus,
        context
    );
    context->tcuStateChangeTimerId = TimerRegisterScheduledTask(
        &HandlerTimerBTTCUStateChange,
        context,
        TIMER_TASK_DISABLED
    );
    EventRegisterCallback(
        BT_EVENT_BOOT,
        &HandlerBTRPI4Boot,
        context
    );
    EventRegisterCallback(
        BT_EVENT_BOOT_STATUS,
        &HandlerBTRPI4BootStatus,
        context
    );
    TimerRegisterScheduledTask(
        &HandlerTimerBTRPI4State,
        context,
        HANDLER_INT_BC127_STATE
    );
    TimerRegisterScheduledTask(
        &HandlerTimerBTRPI4DeviceConnection,
        context,
        HANDLER_INT_DEVICE_CONN
    );
    TimerRegisterScheduledTask(
        &HandlerTimerBTRPI4ScanDevices,
        context,
        HANDLER_INT_DEVICE_SCAN
    );
    TimerRegisterScheduledTask(
        &HandlerTimerBTRPI4Metadata,
        context,
        HANDLER_INT_BT_AVRCP_UPDATER
    );

//    RPI4CommandStatus(context->bt);
/*    if (context->bt->type == BT_BTM_TYPE_BC127) {
        EventRegisterCallback(
            BT_EVENT_BOOT,
            &HandlerBTBC127Boot,
            context
        );
        EventRegisterCallback(
            BT_EVENT_BOOT_STATUS,
            &HandlerBTBC127BootStatus,
            context
        );
        TimerRegisterScheduledTask(
            &HandlerTimerBTBC127State,
            context,
            HANDLER_INT_BC127_STATE
        );
        TimerRegisterScheduledTask(
            &HandlerTimerBTBC127DeviceConnection,
            context,
            HANDLER_INT_DEVICE_CONN
        );
        TimerRegisterScheduledTask(
            &HandlerTimerBTBC127ScanDevices,
            context,
            HANDLER_INT_DEVICE_SCAN
        );
        TimerRegisterScheduledTask(
            &HandlerTimerBTBC127OpenProfileErrors,
            context,
            HANDLER_INT_PROFILE_ERROR
        );
        TimerRegisterScheduledTask(
            &HandlerTimerBTBC127Metadata,
            context,
            HANDLER_INT_BT_AVRCP_UPDATER
        );
        BC127CommandStatus(context->bt);
    } else {
        EventRegisterCallback(
            BT_EVENT_AVRCP_PDU_CHANGE,
            &HandlerBTBM83AVRCPUpdates,
            context
        );
        EventRegisterCallback(
            BT_EVENT_BOOT,
            &HandlerBTBM83Boot,
            context
        );
        EventRegisterCallback(
            BT_EVENT_BOOT_STATUS,
            &HandlerBTBM83BootStatus,
            context
        );
        EventRegisterCallback(
            BT_EVENT_DSP_STATUS,
            &HandlerBTBM83DSPStatus,
            context
        );
        TimerRegisterScheduledTask(
            &HandlerTimerBTBM83ScanDevices,
            context,
            HANDLER_INT_DEVICE_SCAN
        );
        context->avrcpRegisterStatusNotifierTimerId = TimerRegisterScheduledTask(
            &HandlerTimerBTBM83AVRCPManager,
            context,
            HANDLER_INT_BT_AVRCP_UPDATER
        );
        context->bm83PowerStateTimerId = TimerRegisterScheduledTask(
            &HandlerTimerBTBM83ManagePowerState,
            context,
            HANDLER_INT_BM83_POWER_RESET
        );
    }*/
    TimerRegisterScheduledTask(
        &HandlerTimerBTVolumeManagement,
        context,
        HANDLER_INT_VOL_MGMT
    );
}

void HandlerBTCallStatus(void *ctx, uint8_t *data)
{
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_OFF) {
        return;
    }
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->callStatus == BT_CALL_INACTIVE &&
        context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING
    ) {
        BTCommandPlay(context->bt);
    }
    uint8_t statusChange = HandlerSetIBusTELStatus(
        context,
        HANDLER_TEL_STATUS_SET
    );
    if (statusChange == 0) {
        return;
    }
    LogDebug(LOG_SOURCE_SYSTEM, "Call > TCU");
/*    if (context->bt->type == BT_BTM_TYPE_BM83) {
        uint8_t micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
        while (micGain > 0) {
            if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
                BM83CommandMicGainUp(context->bt);
            } else {
                BM83CommandMicGainDown(context->bt);
            }
            micGain--;
        }
    }*/
    if (HandlerGetTelMode(context) == HANDLER_TEL_MODE_TCU) {
        uint8_t boardVersion = UtilsGetBoardVersion();
        if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
            LogDebug(LOG_SOURCE_SYSTEM, "Call > TCU > Begin");
            if (//boardVersion == BOARD_VERSION_TWO &&
                context->ibus->vehicleType != IBUS_VEHICLE_TYPE_E8X
            ) {
//                SPDIF_RST = 0;
                TimerSetTaskInterval(
                    context->tcuStateChangeTimerId,
                    HANDLER_INT_TCU_STATE_CHANGE
                );
            } else {
                HandlerTimerBTTCUStateChange(ctx);
            }
        } else {
            LogDebug(LOG_SOURCE_SYSTEM, "Call > TCU > End");
            //PCM51XXSetVolume(ConfigGetSetting(CONFIG_SETTING_DAC_AUDIO_VOL));
//            PAM_SHDN = 0;
            UtilsSetPinMode(UTILS_PIN_TEL_MUTE, 0);
/*            if (boardVersion == BOARD_VERSION_TWO) {
                SPDIF_RST = 1;
            }*/
        }
    } else {
        uint8_t dspMode = ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC);
        int8_t volume = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
        uint8_t sourceSystem = IBUS_DEVICE_BMBT;
        uint8_t volStepMax = 0x03;
        if (context->ibus->moduleStatus.MID == 1) {
            sourceSystem = IBUS_DEVICE_MID;
        }
        if (context->uiMode == CONFIG_UI_CD53 ||
            context->ibus->vehicleType == IBUS_VEHICLE_TYPE_R50
        ) {
            sourceSystem = IBUS_DEVICE_MFL;
            volStepMax = 0x01;
        }
        if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
            if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING &&
                dspMode == CONFIG_SETTING_DSP_INPUT_SPDIF &&
                context->ibus->moduleStatus.DSP == 1
            ) {
                IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
            }
            LogDebug(LOG_SOURCE_SYSTEM, "Call > Begin");
            if (strlen(context->bt->callerId) > 0 &&
                context->uiMode != CONFIG_UI_CD53
            ) {
                IBusCommandTELStatusText(context->ibus, context->bt->callerId, 0);
            }
            if (volume > CONFIG_SETTING_TEL_VOL_OFFSET_MAX) {
                volume = CONFIG_SETTING_TEL_VOL_OFFSET_MAX;
                ConfigSetSetting(CONFIG_SETTING_TEL_VOL, CONFIG_SETTING_TEL_VOL_OFFSET_MAX);
            }
            LogDebug(LOG_SOURCE_SYSTEM, "Call > Volume: %+d", volume);
            uint8_t direction = 1;
            if (volume < 0) {
                direction = 0;
                volume = -volume;
            }
            while (volume > 0) {
                uint8_t volStep = volume;
                if (volStep > volStepMax) {
                    volStep = volStepMax;
                }
                IBusCommandSetVolume(
                    context->ibus,
                    sourceSystem,
                    IBUS_DEVICE_RAD,
                    (volStep << 4) | direction
                );
                volume = volume - volStep;
            }
        } else {
            LogDebug(LOG_SOURCE_SYSTEM, "Call > End");
            UtilsStrncpy(
                context->bt->callerId,
                LocaleGetText(LOCALE_STRING_VOICE_ASSISTANT),
                BT_CALLER_ID_FIELD_SIZE
            );
            context->telStatus = HANDLER_TEL_STATUS_VOL_CHANGE;
            LogDebug(LOG_SOURCE_SYSTEM, "Call > Volume: %+d", -volume);
            uint8_t direction = 0;
            if (volume < 0) {
                direction = 1;
                volume = -volume;
            }
            while (volume > 0) {
                uint8_t volStep = volume;
                if (volStep > volStepMax) {
                    volStep = volStepMax;
                }
                IBusCommandSetVolume(
                    context->ibus,
                    sourceSystem,
                    IBUS_DEVICE_RAD,
                    (volStep << 4) | direction
                );
                volume = volume - volStep;
            }
            IBusCommandBlueBusSetStatus(
                context->ibus,
                IBUS_BLUEBUS_SUBCMD_SET_STATUS_TEL,
                IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE
            );
            if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING &&
                dspMode == CONFIG_SETTING_DSP_INPUT_SPDIF &&
                context->ibus->moduleStatus.DSP == 1
            ) {
                IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
            }
        }
    }
}

void HandlerBTDeviceFound(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->status == BT_STATUS_DISCONNECTED
//        && context->ibus->ignitionStatus > IBUS_IGNITION_OFF
    ) {
        LogDebug(LOG_SOURCE_SYSTEM, "Handler: No Device -- Attempt connection");
        if (0) {//context->bt->type == BT_BTM_TYPE_BC127) {
            memcpy(context->bt->activeDevice.macId, data, BT_MAC_ID_LEN);
            //BC127CommandProfileOpen(context->bt, "A2DP");
        } else {
            if (context->bt->pairedDevicesCount > 0) {
                if (context->btSelectedDevice == HANDLER_BT_SELECTED_DEVICE_NONE ||
                    context->bt->pairedDevicesCount == 1
                ) {
                    BTPairedDevice_t *dev = &context->bt->pairedDevices[0];
                    BTCommandConnect(context->bt, dev);
                    context->btSelectedDevice = 0;
                } else {
                    if (context->btSelectedDevice + 1 < context->bt->pairedDevicesCount) {
                        context->btSelectedDevice++;
                    } else {
                        context->btSelectedDevice = 0;
                    }
                    BTPairedDevice_t *dev = &context->bt->pairedDevices[context->btSelectedDevice];
                    BTCommandConnect(context->bt, dev);
                }
            }
        }
    } else {
        LogDebug(
            LOG_SOURCE_SYSTEM,
            "Handler: Not connecting to new device %d %d %d",
            context->bt->activeDevice.deviceId,
            context->bt->status,
            context->ibus->ignitionStatus
        );
    }
}


void HandlerBTCallerID(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
        LogDebug(LOG_SOURCE_SYSTEM, "Call > ID: %s", context->bt->callerId);
        IBusCommandTELStatusText(context->ibus, context->bt->callerId, 0);
    }
}

void HandlerBTDeviceLinkConnected(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;

    if (1) {//context->ibus->ignitionStatus > IBUS_IGNITION_OFF) {
        uint8_t linkType = *data;
        uint8_t hfpConfigStatus = ConfigGetSetting(CONFIG_SETTING_HFP);

        if (linkType == BT_LINK_TYPE_A2DP &&
            context->bt->activeDevice.a2dpId != 0
        ) {
            /*if (context->bt->type == BT_BTM_TYPE_BC127) {
                BC127CommandVolume(
                    context->bt,
                    context->bt->activeDevice.a2dpId,
                    "UP"
                );
                if (hfpConfigStatus == CONFIG_SETTING_ON &&
                    context->bt->activeDevice.hfpId == 0
                ) {
                    BC127CommandProfileOpen(
                        context->bt,
                        "HFP"
                    );
                }
            }*/
        }
        if (linkType == BT_LINK_TYPE_AVRCP || linkType == BT_LINK_TYPE_A2DP) {
            if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_ON &&
                context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING
            ) {
                BTCommandPlay(context->bt);
            }
            if (1) {//context->bt->type == BT_BTM_TYPE_BM83) {
                char tmp[BT_DEVICE_NAME_LEN] = {0};
                if (memcmp(tmp, context->bt->activeDevice.deviceName, BT_DEVICE_NAME_LEN) == 0) {
                    ConfigSetSetting(
                        CONFIG_SETTING_LAST_CONNECTED_DEVICE,
                        context->btSelectedDevice
                    );
                    /*BM83CommandReadLinkedDeviceInformation(
                        context->bt,
                        BM83_LINKED_DEVICE_QUERY_NAME
                    );*/
                }
            }
        }
        if (linkType == BT_LINK_TYPE_HFP) {
            HandlerSetIBusTELStatus(context, HANDLER_TEL_STATUS_FORCE);
            if (hfpConfigStatus == CONFIG_SETTING_OFF) {
                /*if (context->bt->type == BT_BTM_TYPE_BM83) {
                    BM83CommandDisconnect(context->bt, BM83_CMD_DISCONNECT_PARAM_HF);
                } else {
                    BC127CommandClose(context->bt, context->bt->activeDevice.hfpId);
                }*/
            } else {
                IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_GREEN);
                /*if (context->bt->type == BT_BTM_TYPE_BC127) {
                    BC127CommandATSet(context->bt, "CSCS", "\"UTF-8\"");
                    BC127CommandATSet(context->bt, "CLIP", "1");
                    BC127CommandAT(context->bt, "+CCLK?");
                    if (context->bt->activeDevice.hfpId != 0 &&
                        context->bt->activeDevice.pbapId == 0
                    ) {
                        BC127CommandProfileOpen(
                            context->bt,
                            "PBAP"
                        );
                    }
                }*/
            }
        }
    } else {
        BTCommandDisconnect(context->bt);
    }
}

void HandlerBTDeviceDisconnected(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BTClearMetadata(context->bt);
    /*if (context->bt->type == BT_BTM_TYPE_BC127) {
        BC127ClearPairingErrors(context->bt);
    }*/
    if (context->ibus->ignitionStatus > IBUS_IGNITION_OFF) {
        if (context->btSelectedDevice != HANDLER_BT_SELECTED_DEVICE_NONE) {
            BTPairedDevice_t *dev = &context->bt->pairedDevices[
                context->btSelectedDevice
            ];
            BTCommandConnect(context->bt, dev);
        } else {
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
                IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_RED);
            }
            BTCommandList(context->bt);
        }
    }
}

void HandlerBTPlaybackStatus(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->btStartupIsRun == 0) {
        /*if (context->bt->type == BT_BTM_TYPE_BC127) {
            if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
                BTCommandGetMetadata(context->bt);
            }
        } else {
            context->bt->avrcpUpdates = SET_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_GET_METADATA
            );
            context->bt->avrcpUpdates = SET_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF
            );
            TimerResetScheduledTask(context->avrcpRegisterStatusNotifierTimerId);
        }*/
        context->btStartupIsRun = 1;
    }
    if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING &&
        context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING
    ) {
        BTCommandPause(context->bt);
    }
}

void HandlerBTTimeUpdate(void *ctx, uint8_t *dt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
/*    if (dt[BC127_AT_DATE_SEC] < 2) {
        LogDebug(
            LOG_SOURCE_BT,
            "Setting time from BT: 20%d-%.2d-%.2d %.2d:%.2d",
            dt[BC127_AT_DATE_YEAR],
            dt[BC127_AT_DATE_MONTH],
            dt[BC127_AT_DATE_DAY],
            dt[BC127_AT_DATE_HOUR],
            dt[BC127_AT_DATE_MIN]
        );
        IBusCommandIKESetDate(
            context->ibus,
            dt[BC127_AT_DATE_YEAR],
            dt[BC127_AT_DATE_MONTH],
            dt[BC127_AT_DATE_DAY]
        );
        IBusCommandIKESetTime(context->ibus, dt[BC127_AT_DATE_HOUR], dt[BC127_AT_DATE_MIN]);
    } else if (dt[BC127_AT_DATE_SEC] < 60) {
        TimerRegisterScheduledTask(
            &HandlerTimerBTBC127RequestDateTime,
            ctx,
            (60 - dt[5]) * 1000
        );
    }*/

}

/*void HandlerBTBC127Boot(void *ctx, uint8_t *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BTClearPairedDevices(context->bt, BT_TYPE_CLEAR_ALL);
    //BC127CommandStatus(context->bt);
}

void HandlerBTBC127BootStatus(void *ctx, uint8_t *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BC127CommandList(context->bt);
    if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF) {
        BC127CommandBtState(context->bt, BT_STATE_OFF, BT_STATE_OFF);
        BC127CommandClose(context->bt, BT_CLOSE_ALL);
    } else {
        BC127CommandBtState(context->bt, BT_STATE_ON, context->bt->discoverable);
    }
}

void HandlerBTBM83AVRCPUpdates(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t type = data[0];
    uint8_t status = data[1];
    if ((type == BM83_AVRCP_EVT_PLAYBACK_STATUS_CHANGED &&
        status == BM83_AVRCP_DATA_PLAYBACK_STATUS_PLAYING) ||
        type == BM83_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED
    ) {
        context->bt->avrcpUpdates = SET_BIT(
            context->bt->avrcpUpdates,
            BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF
        );
        if (status == BM83_AVRCP_DATA_PLAYBACK_STATUS_PLAYING) {
            context->bt->avrcpUpdates = SET_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_GET_METADATA
            );
        }
        TimerResetScheduledTask(context->avrcpRegisterStatusNotifierTimerId);
    } else if (type == BM83_AVRCP_EVT_PLAYBACK_TRACK_CHANGED) {
        if (status != BM83_DATA_AVC_RSP_INTERIM) {
            context->bt->avrcpUpdates = SET_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF
            );
            context->bt->avrcpUpdates = SET_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_GET_METADATA
            );
            TimerResetScheduledTask(context->avrcpRegisterStatusNotifierTimerId);
        }
    }
}

void HandlerBTBM83DSPStatus(void *ctx, uint8_t *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t sampleRate = pkt[0];
    if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF ||
        context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E8X
    ) {
        return;
    }
    if (sampleRate != BM83_DATA_DSP_REPORTED_SR_44_1kHz) {
        LogDebug(LOG_SOURCE_BT, "Disable S/PDIF");
        SPDIF_RST = 0;
    } else {
        SPDIF_RST = 1;
    }
}

void HandlerBTBM83Boot(void *ctx, uint8_t *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    //BM83CommandPowerOn(context->bt);
    TimerUnregisterScheduledTaskById(context->bm83PowerStateTimerId);
}

void HandlerBTBM83BootStatus(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t type = *data;
    if (type == BM83_DATA_BOOT_STATUS_POWER_ON) {
        BM83CommandReadPairedDevices(context->bt);
    }
}*/

void HandlerTimerBTTCUStateChange(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
        LogDebug(LOG_SOURCE_SYSTEM, "Call > TCU > Enable");
        if (ConfigGetSetting(CONFIG_SETTING_TEL_MODE) != CONFIG_SETTING_TEL_MODE_NO_MUTE) {
            UtilsSetPinMode(UTILS_PIN_TEL_MUTE, 1);
        }
        //PCM51XXSetVolume(ConfigGetSetting(CONFIG_SETTING_DAC_TEL_TCU_MODE_VOL));
        //PAM_SHDN = 1;
    }
    TimerSetTaskInterval(
        context->tcuStateChangeTimerId,
        TIMER_TASK_DISABLED
    );
}

void HandlerTimerBTVolumeManagement(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    /*if (ConfigGetSetting(CONFIG_SETTING_MANAGE_VOLUME) == CONFIG_SETTING_ON &&
        context->volumeMode != HANDLER_VOLUME_MODE_LOWERED &&
        context->bt->activeDevice.a2dpId != 0 &&
        context->bt->type != BT_BTM_TYPE_BM83 &&
        context->bt->activeDevice.a2dpVolume != 0
    ) {
        if (context->bt->activeDevice.a2dpVolume < 127) {
            LogWarning(
                "BT: Set Max Volume (%d)",
                context->bt->activeDevice.a2dpVolume
            );
            BC127CommandVolume(context->bt, context->bt->activeDevice.a2dpId, "F");
            context->bt->activeDevice.a2dpVolume = 127;
        }
    }
    uint8_t lowerVolumeOnReverse = ConfigGetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV);
    uint32_t now = TimerGetMillis();
    if (lowerVolumeOnReverse == CONFIG_SETTING_ON &&
        context->ibus->moduleStatus.PDC == 1 &&
        context->bt->activeDevice.a2dpId != 0
    ) {
        uint32_t timeSinceUpdate = now - context->pdcLastStatus;
        if (context->volumeMode == HANDLER_VOLUME_MODE_LOWERED &&
            timeSinceUpdate >= HANDLER_WAIT_REV_VOL
        ) {
            LogWarning(
                "PDC DONE - RAISE VOLUME -- Currently %d",
                context->bt->activeDevice.a2dpVolume
            );
            HandlerSetVolume(context, HANDLER_VOLUME_DIRECTION_UP);
        }
        if (context->volumeMode == HANDLER_VOLUME_MODE_NORMAL &&
            timeSinceUpdate <= HANDLER_WAIT_REV_VOL
        ) {
            LogWarning(
                "PDC START - LOWER VOLUME -- Currently %d",
                context->bt->activeDevice.a2dpVolume
            );
            HandlerSetVolume(context, HANDLER_VOLUME_DIRECTION_DOWN);
        }
    }
    if (lowerVolumeOnReverse == CONFIG_SETTING_ON &&
        context->bt->activeDevice.a2dpId != 0
    ) {
        uint32_t timeSinceUpdate = now - context->gearLastStatus;
        if (context->volumeMode == HANDLER_VOLUME_MODE_LOWERED &&
            context->ibus->gearPosition != IBUS_IKE_GEAR_REVERSE &&
            timeSinceUpdate >= HANDLER_WAIT_REV_VOL
        ) {
            LogWarning(
                "TRANS OUT OF REV - RAISE VOLUME -- Currently %d",
                context->bt->activeDevice.a2dpVolume
            );
            HandlerSetVolume(context, HANDLER_VOLUME_DIRECTION_UP);
        }
        if (context->volumeMode == HANDLER_VOLUME_MODE_NORMAL &&
            context->ibus->gearPosition == IBUS_IKE_GEAR_REVERSE &&
            timeSinceUpdate >= HANDLER_WAIT_REV_VOL
        ) {
            LogWarning(
                "TRANS IN REV - LOWER VOLUME -- Currently %d",
                context->bt->activeDevice.a2dpVolume
            );
            HandlerSetVolume(context, HANDLER_VOLUME_DIRECTION_DOWN);
        }
    }*/
}

/*void HandlerTimerBTBC127State(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->powerState == BT_STATE_OFF &&
        context->btBootState == HANDLER_BT_BOOT_OK
    ) {
        LogWarning("BC127 Boot Failure");
        uint16_t bootFailCount = ConfigGetBC127BootFailures();
        bootFailCount++;
        //ConfigSetBC127BootFailures(bootFailCount);
        IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_RED_BLINKING);
        context->btBootState = HANDLER_BT_BOOT_FAIL;
    }
}

void HandlerTimerBTBC127DeviceConnection(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (BTHasActiveMacId(context->bt) != 0 && context->bt->activeDevice.a2dpId == 0) {
        if (context->btDeviceConnRetries <= HANDLER_DEVICE_MAX_RECONN) {
            LogDebug(
                LOG_SOURCE_SYSTEM,
                "Handler: A2DP link closed -- Attempting to connect"
            );
            BC127CommandProfileOpen(
                context->bt,
                "A2DP"
            );
            context->btDeviceConnRetries += 1;
        } else {
            LogError("Handler: Giving up on BT connection");
            context->btDeviceConnRetries = 0;
            BTClearPairedDevices(context->bt, BT_TYPE_CLEAR_ALL);
            BTCommandDisconnect(context->bt);
        }
    } else if (context->btDeviceConnRetries > 0) {
        context->btDeviceConnRetries = 0;
    }
}

void HandlerTimerBTBC127RequestDateTime(void *ctx) {
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    TimerUnregisterScheduledTask(&HandlerTimerBTBC127RequestDateTime);
    //BC127CommandAT(context->bt, "+CCLK?");
}

void HandlerTimerBTBC127OpenProfileErrors(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (BTHasActiveMacId(context->bt) != 0) {
        for (uint8_t idx = 0; idx < BC127_PROFILE_COUNT; idx++) {
            if (context->bt->pairingErrors[idx] == 1 && PROFILES[idx] != 0) {
                LogDebug(LOG_SOURCE_SYSTEM, "Handler: Attempting to resolve pairing error");
                BC127CommandProfileOpen(
                    context->bt,
                    PROFILES[idx]
                );
                context->bt->pairingErrors[idx] = 0;
            }
        }
    }
}

void HandlerTimerBTBC127ScanDevices(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (((context->bt->activeDevice.deviceId == 0 &&
        context->bt->status == BT_STATUS_DISCONNECTED) ||
        context->scanIntervals == 12) &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF
    ) {
        context->scanIntervals = 0;
        //BC127CommandList(context->bt);
    } else {
        context->scanIntervals += 1;
    }

    if (context->bt->status != BT_STATUS_DISCONNECTED &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF &&
        context->bt->activeDevice.avrcpId != 0
    ) {
        //BC127CommandStatusAVRCP(context->bt);
    }
}

void HandlerTimerBTBC127Metadata(HandlerContext_t *context)
{
    uint32_t now = TimerGetMillis();
    if (now - HANDLER_BT_METADATA_TIMEOUT >= context->bt->metadataTimestamp &&
        context->bt->activeDevice.avrcpId != 0 &&
        context->bt->callStatus == BT_CALL_INACTIVE &&
        context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING
    ) {
        //BC127CommandGetMetadata(context->bt);
    }
}

void HandlerTimerBTBM83AVRCPManager(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->avrcpUpdates != 0x00) {
        if (CHECK_BIT(context->bt->avrcpUpdates, BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF) > 0) {
            context->bt->avrcpUpdates = CLEAR_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF
            );
            BM83CommandAVRCPRegisterNotification(
                context->bt,
                BM83_AVRCP_EVT_PLAYBACK_TRACK_CHANGED
            );
        } else if (CHECK_BIT(context->bt->avrcpUpdates, BT_AVRCP_ACTION_GET_METADATA) > 0) {
            context->bt->avrcpUpdates = CLEAR_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_GET_METADATA
            );
            //BM83CommandAVRCPGetElementAttributesAll(context->bt);
        }
        if (CHECK_BIT(context->bt->avrcpUpdates, BT_AVRCP_ACTION_GET_METADATA) > 0) {
            TimerSetTaskInterval(
                context->avrcpRegisterStatusNotifierTimerId,
                HANDLER_INT_BT_AVRCP_UPDATER_METADATA
            );
        } else {
            TimerSetTaskInterval(
                context->avrcpRegisterStatusNotifierTimerId,
                HANDLER_INT_BT_AVRCP_UPDATER
            );
        }
    }
}


void HandlerTimerBTBM83AVRCPPlaybackState(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->activeDevice.avrcpId != 0) {
    }
}

void HandlerTimerBTBM83ManagePowerState(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->powerState == BT_STATE_OFF) {
        switch (context->btBootState) {
            case HANDLER_BT_BOOT_RESET: {
                BT_RST = 1;
                BT_MFB = 1;
                BT_RST_MODE = 1;
                context->btBootState = HANDLER_BT_BOOT_MFB_H;
                TimerSetTaskInterval(context->bm83PowerStateTimerId, HANDLER_INT_BM83_POWER_MFB_ON);
                break;
            }
            case HANDLER_BT_BOOT_MFB_H: {
                BT_MFB = 0;
                context->btBootState = HANDLER_BT_BOOT_MFB_L;
                TimerSetTaskInterval(context->bm83PowerStateTimerId, HANDLER_INT_BM83_POWER_MFB_OFF);
                break;
            }
            case HANDLER_BT_BOOT_MFB_L: {
                BT_RST_MODE = 0;
                BT_RST = 0;
                BT_MFB = 1;
                context->btBootState = HANDLER_BT_BOOT_RESET;
                TimerSetTaskInterval(context->bm83PowerStateTimerId, HANDLER_INT_BM83_POWER_RESET);
                break;
            }
        }
    }
}

void HandlerTimerBTBM83ScanDevices(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->ibus->ignitionStatus > IBUS_IGNITION_OFF) {
        if (context->bt->pairedDevicesCount == 0 &&
            context->bt->powerState == BT_STATE_STANDBY
        ) {
            //BM83CommandReadPairedDevices(context->bt);
        }
        if (context->bt->status == BT_STATUS_DISCONNECTED &&
            context->bt->discoverable == BT_STATE_OFF &&
            context->bt->pairedDevicesCount > 0
        ) {
            if (context->btSelectedDevice == HANDLER_BT_SELECTED_DEVICE_NONE ||
                context->bt->pairedDevicesCount == 1
            ) {
                BTPairedDevice_t *dev = &context->bt->pairedDevices[0];
                BTCommandConnect(context->bt, dev);
                context->btSelectedDevice = 0;
            } else {
                if (context->btSelectedDevice + 1 < context->bt->pairedDevicesCount) {
                    context->btSelectedDevice++;
                } else {
                    context->btSelectedDevice = 0;
                }
                BTPairedDevice_t *dev = &context->bt->pairedDevices[context->btSelectedDevice];
                BTCommandConnect(context->bt, dev);
            }
        }
    }
}*/

/**
 * HandlerBTRPI4Boot()
 *     Description:
 *         If the RPI4 module restarts, reset our internal state
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBTRPI4Boot(void *ctx, uint8_t *tmp)
{
    LogDebug(LOG_SOURCE_BT, "BTRPI4Boot");
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BTClearPairedDevices(context->bt, BT_TYPE_CLEAR_ALL);
    RPI4CommandStatus(context->bt);
}

/**
 * HandlerBTRPI4BootStatus()
 *     Description:
 *         If the RPI4 Radios are off, meaning we rebooted and got the status
 *         back, then alter the module status to match the ignition status
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBTRPI4BootStatus(void *ctx, uint8_t *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    RPI4CommandList(context->bt);
/*    if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF) {
        // Set the BT module not connectable or discoverable and disconnect all devices
        RPI4CommandBtState(context->bt, BT_STATE_OFF, BT_STATE_OFF);
        RPI4CommandDisconnect(context->bt);
    } else {
        // Set the connectable and discoverable states to what they were
        RPI4CommandBtState(context->bt, BT_STATE_ON, context->bt->discoverable);
    }*/
}

/* Timers */

/**
 * HandlerTimerBTRPI4State()
 *     Description:
 *         Ensure the RPI4 has booted, and if not, blink the red TEL LED
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBTRPI4State(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->powerState == BT_STATE_OFF &&
        context->btBootState == HANDLER_BT_BOOT_OK
    ) {
        LogWarning("RPI4 Boot Failure");
        uint16_t bootFailCount = ConfigGetBC127BootFailures();
        bootFailCount++;
        ConfigSetBC127BootFailures(bootFailCount);
        IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_RED_BLINKING);
        context->btBootState = HANDLER_BT_BOOT_FAIL;
    }
}

/**
 * HandlerTimerBTRPI4DeviceConnection()
 *     Description:
 *         Monitor the BT connection and ensure it stays connected
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBTRPI4DeviceConnection(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (BTHasActiveMacId(context->bt) != 0 && context->bt->activeDevice.a2dpId == 0) {
        if (context->btDeviceConnRetries <= HANDLER_DEVICE_MAX_RECONN) {
            uint8_t idx;
            for (idx = 0; idx < context->bt->pairedDevicesCount; idx++) {
                BTPairedDevice_t *btDevice = &context->bt->pairedDevices[idx];
                if (memcmp(context->bt->activeDevice.macId, btDevice->macId, BT_MAC_ID_LEN) == 0) {
                    LogDebug(LOG_SOURCE_SYSTEM,
                        "Handler: A2DP link closed -- Attempting to connect"
                    );
                    RPI4CommandConnect(context->bt, btDevice);
                    context->btDeviceConnRetries += 1;
                }
            }
        } else {
            LogError("Handler: Giving up on BT connection");
            context->btDeviceConnRetries = 0;
            BTClearPairedDevices(context->bt, BT_TYPE_CLEAR_ALL);
            BTCommandDisconnect(context->bt);
        }
    } else if (context->btDeviceConnRetries > 0) {
        context->btDeviceConnRetries = 0;
    }
}

/**
 * HandlerTimerBTRPI4ScanDevices()
 *     Description:
 *         Rescan for devices on the PDL periodically. Scan every 5 seconds if
 *         there is no connected device, otherwise every 60 seconds
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBTRPI4ScanDevices(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (((context->bt->activeDevice.deviceId == 0 &&
        context->bt->status == BT_STATUS_DISCONNECTED) ||
        context->scanIntervals == 12) &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF
    ) {
        context->scanIntervals = 0;
        RPI4CommandList(context->bt);
    } else {
        context->scanIntervals += 1;
    }

    if (context->bt->status != BT_STATUS_DISCONNECTED &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF &&
        context->bt->activeDevice.avrcpId != 0
    ) {
        // Sync the playback state every 5 seconds
        RPI4CommandStatusAVRCP(context->bt);
    }
}

/**
 * HandlerTimerBTRPI4Metadata()
 *     Description:
 *         Request current playing song periodically
 *     Params:
 *         HandlerContext_t *context - The handler context
 *     Returns:
 *         void
 */
void HandlerTimerBTRPI4Metadata(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t*) ctx;
    uint32_t now = TimerGetMillis();
    if (now - HANDLER_BT_METADATA_TIMEOUT >= context->bt->metadataTimestamp &&
        context->bt->activeDevice.avrcpId != 0 &&
        context->bt->callStatus == BT_CALL_INACTIVE &&
        context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING
    ) {
        RPI4CommandGetMetadata(context->bt);
    }
}
