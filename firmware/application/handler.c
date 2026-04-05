#include <bluetooth/bluetooth.h>
#include <stdint.h>
#include "handler.h"
static HandlerContext_t Context;

void HandlerInit(BT_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    uint32_t now = TimerGetMillis();
    Context.btDeviceConnRetries = 0;
    Context.btStartupIsRun = 0;
    Context.btSelectedDevice = HANDLER_BT_SELECTED_DEVICE_NONE;
    Context.volumeMode = HANDLER_VOLUME_MODE_NORMAL;
    Context.gtStatus = HANDLER_GT_STATUS_UNCHECKED;
    Context.monitorStatus = HANDLER_MONITOR_STATUS_UNSET;
    Context.uiMode = ConfigGetUIMode();
    Context.seekMode = HANDLER_CDC_SEEK_MODE_NONE;
    Context.lmDimmerChecksum = 0x00;
    Context.mflButtonStatus = HANDLER_MFL_STATUS_OFF;
    Context.telStatus = IBUS_TEL_STATUS_NONE;
    Context.btBootState = HANDLER_BT_BOOT_OK;
    memset(&Context.gmState, 0, sizeof(HandlerBodyModuleStatus_t));
    memset(&Context.lmState, 0, sizeof(HandlerLightControlStatus_t));
    Context.powerStatus = HANDLER_POWER_ON;
    Context.scanIntervals = 0;
    Context.lmLastIOStatus = 0;
    Context.cdChangerLastPoll = now;
    Context.cdChangerLastStatus = now;
    Context.pdcLastStatus = 0;
    Context.lmLastStatusSet = 0;
    Context.radLastMessage = TimerGetMillis();
    EventRegisterCallback(
        UIEvent_CloseConnection,
        &HandlerUICloseConnection,
        &Context
    );
    EventRegisterCallback(
        UIEvent_InitiateConnection,
        &HandlerUIInitiateConnection,
        &Context
    );
    TimerRegisterScheduledTask(
        &HandlerTimerPoweroff,
        &Context,
        HANDLER_INT_POWEROFF
    );
    HandlerBTInit(&Context);
    HandlerIBusInit(&Context);
    if (Context.uiMode == CONFIG_UI_CD53 ||
        Context.uiMode == CONFIG_UI_MIR ||
        Context.uiMode == CONFIG_UI_IRIS
    ) {
        //CD53Init(bt, ibus);
    } else if (Context.uiMode == CONFIG_UI_BMBT) {
        BMBTInit(bt, ibus);
    } else if (Context.uiMode == CONFIG_UI_MID) {
        //MIDInit(bt, ibus);
    } else if (Context.uiMode == CONFIG_UI_MID_BMBT) {
        //MIDInit(bt, ibus);
        BMBTInit(bt, ibus);
    }
}

void HandlerUICloseConnection(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BTClearMetadata(context->bt);
    BTClearActiveDevice(context->bt);
    BTCommandDisconnect(context->bt);
}

void HandlerUIInitiateConnection(void *ctx, unsigned char *deviceId)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->activeDevice.deviceId != 0) {
        BTCommandDisconnect(context->bt);
    }
    context->btSelectedDevice = (int8_t) *deviceId;
    ConfigSetBytes(CONFIG_SETTING_LAST_CONNECTED_DEVICE_MAC,(uint8_t *)&context->bt->pairedDevices[context->btSelectedDevice].macId, BT_MAC_ID_LEN);
    BTCommandSetConnectable(context->bt, BT_STATE_ON);
}

void HandlerTimerPoweroff(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (ConfigGetSetting(CONFIG_SETTING_AUTO_POWEROFF) == CONFIG_SETTING_ON) {
        uint32_t lastRx = TimerGetMillis() - context->ibus->rxLastStamp;
        if (lastRx >= HANDLER_POWER_TIMEOUT_MILLIS) {
            if (context->powerStatus == HANDLER_POWER_ON) {
                //UARTDestroy(IBUS_UART_MODULE);
                TimerDelayMicroseconds(500);
                context->powerStatus = HANDLER_POWER_OFF;
                //IBUS_EN = 0;
            } else {
                //IBUS_EN = 1;
                context->powerStatus = HANDLER_POWER_ON;
            }
        }
    }
}
