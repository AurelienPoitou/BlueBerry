#include <stdint.h>
#include "handler_common.h"

uint8_t HandlerGetTelMode(HandlerContext_t *context)
{
    uint8_t dspMode = ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC);
    uint8_t telMode = ConfigGetSetting(CONFIG_SETTING_TEL_MODE);
    if ((context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING ||
        (dspMode == CONFIG_SETTING_DSP_INPUT_SPDIF && context->ibus->moduleStatus.DSP == 1)) &&
        telMode == CONFIG_SETTING_TEL_MODE_DEFAULT
    ) {
        return HANDLER_TEL_MODE_AUDIO;
    } else {
        return HANDLER_TEL_MODE_TCU;
    }
}

uint8_t HandlerSetIBusTELStatus(
    HandlerContext_t *context,
    unsigned char sendFlag
) {
    if (ConfigGetTelephonyFeaturesActive() == CONFIG_SETTING_ON) {
        unsigned char currentTelStatus = 0x00;
        if (context->bt->scoStatus == BT_CALL_SCO_OPEN) {
            currentTelStatus = IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE;
        } else {
            currentTelStatus = IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE;
        }
        if (context->telStatus != currentTelStatus ||
            sendFlag == HANDLER_TEL_STATUS_FORCE
        ) {
            context->telStatus = currentTelStatus;
            if (currentTelStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE &&
                (
                    context->uiMode == CONFIG_UI_CD53 ||
                    context->uiMode == CONFIG_UI_MIR ||
                    context->uiMode == CONFIG_UI_IRIS ||
                    context->ibus->vehicleType == IBUS_VEHICLE_TYPE_R50
                ) &&
                context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING
            ) {
                return 1;
            }
            IBusCommandTELStatus(context->ibus, currentTelStatus);
            return 1;
        }
    }
    return 0;
}

void HandlerSetVolume(HandlerContext_t *context, uint8_t direction)
{
    uint8_t newVolume = 0;
    if (direction == HANDLER_VOLUME_DIRECTION_DOWN) {
        newVolume = context->bt->activeDevice.a2dpVolume / 2;
        context->volumeMode = HANDLER_VOLUME_MODE_LOWERED;
    } else {
        newVolume = context->bt->activeDevice.a2dpVolume * 2;
        context->volumeMode = HANDLER_VOLUME_MODE_NORMAL;
    }
    char hexVolString[3] = {0};
    snprintf(hexVolString, 3, "%X", newVolume / 8);
/*    BC127CommandVolume(
        context->bt,
        context->bt->activeDevice.a2dpId,
        hexVolString
    );*/
    context->bt->activeDevice.a2dpVolume = newVolume;
}
