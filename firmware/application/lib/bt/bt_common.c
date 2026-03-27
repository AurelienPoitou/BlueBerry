#include <stdint.h>
//#include "../utils.c"
#include "bt_common.h"


void BTClearActiveDevice(BT_t *bt)
{
    bt->activeDevice = BTConnectionInit();
}

void BTClearMetadata(BT_t *bt)
{
    memset(bt->title, 0, BT_METADATA_FIELD_SIZE);
    memset(bt->artist, 0, BT_METADATA_FIELD_SIZE);
    memset(bt->album, 0, BT_METADATA_FIELD_SIZE);
}

void BTClearPairedDevices(BT_t *bt, uint8_t clearType)
{
    uint8_t idx;
    uint8_t found = 0;
    BTPairedDevice_t btActiveConn;
    for (idx = 0; idx < bt->pairedDevicesCount; idx++) {
        BTPairedDevice_t *btConn = &bt->pairedDevices[idx];
        if (btConn != 0) {
            if (clearType != BT_TYPE_CLEAR_ALL &&
                memcmp(btConn->macId, bt->activeDevice.macId, 6) == 0
            ) {
                memcpy(&btActiveConn, btConn, sizeof(BTPairedDevice_t));
                found = 1;
            }
            memset(btConn, 0, sizeof(bt->pairedDevices[idx]));
        }
    }
    bt->pairedDevicesCount = 0;
    memset(bt->pairingErrors, 0, sizeof(bt->pairingErrors));
    if ((clearType != BT_TYPE_CLEAR_ALL) && (found == 1)) {
        BTPairedDeviceInit(bt, btActiveConn.macId, btActiveConn.deviceName, btActiveConn.number);
    }
}

BTConnection_t BTConnectionInit()
{
    BTConnection_t conn;
    memset(&conn, 0, sizeof(BTConnection_t));
    conn.a2dpVolume = 64;
    return conn;
}


void BTPairedDeviceInit(
    BT_t *bt,
    uint8_t *macId,
    char *deviceName,
    uint8_t deviceNumber
) {
    uint8_t deviceExists = 0;
    uint8_t idx;
    for (idx = 0; idx < bt->pairedDevicesCount; idx++) {
        BTPairedDevice_t *btDevice = &bt->pairedDevices[idx];
        if (memcmp(macId, btDevice->macId, BT_MAC_ID_LEN) == 0) {
            deviceExists = 1;
            EventTriggerCallback(BT_EVENT_DEVICE_FOUND, (uint8_t *) macId);
        }
    }
    if (deviceExists == 0) {
        BTPairedDevice_t pairedDevice;
        memcpy(pairedDevice.macId, macId, BT_MAC_ID_LEN);
        memset(pairedDevice.deviceName, 0, BT_DEVICE_NAME_LEN);
        UtilsStrncpy(pairedDevice.deviceName, deviceName, BT_DEVICE_NAME_LEN);
        if (deviceNumber > 0 && deviceNumber <= BT_MAX_DEVICE_PAIRED) {
            pairedDevice.number = deviceNumber;
            LogDebug(LOG_SOURCE_BT, "Add PD: %d", deviceNumber - 1);
            bt->pairedDevices[deviceNumber - 1] = pairedDevice;
            bt->pairedDevicesCount++;
            EventTriggerCallback(BT_EVENT_DEVICE_FOUND, (uint8_t *) macId);
            LogDebug(LOG_SOURCE_BT, "BT: Rewrite Pairing Profile");
        } else if (bt->pairedDevicesCount+1 < BT_MAX_DEVICE_PAIRED) {
            pairedDevice.number = bt->pairedDevicesCount + 1;
            bt->pairedDevices[bt->pairedDevicesCount++] = pairedDevice;
            EventTriggerCallback(BT_EVENT_DEVICE_FOUND, (uint8_t *) macId);
            LogDebug(LOG_SOURCE_BT, "BT: New Pairing Profile");
        } else {
            LogDebug(LOG_SOURCE_BT, "BT: Ignoring Pairing Profile");
        }
    }
}

char *BTPairedDeviceGetName(BT_t *bt, uint8_t *macId)
{
    char *deviceName = 0;
    uint8_t idx;
    for (idx = 0; idx <= bt->pairedDevicesCount; idx++) {
        BTPairedDevice_t *btDevice = &bt->pairedDevices[idx];
        if (memcmp(macId, btDevice->macId, 6) == 0) {
            deviceName = btDevice->deviceName;
        }
    }
    return deviceName;
}
