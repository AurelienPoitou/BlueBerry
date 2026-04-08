#include "ibus.h"
#include "ibus_pretty.h"
#include "log.h"

typedef struct {
    uint8_t id;
    const char *name;
} IBusDeviceName_t;

static const IBusDeviceName_t ibusDeviceNames[] = {
    { IBUS_DEVICE_GM,   "GM" },
    { IBUS_DEVICE_CDC,  "CDC" },
    { IBUS_DEVICE_FUH,  "FUH" },
    { IBUS_DEVICE_CCM,  "CCM" },
    { IBUS_DEVICE_GT,   "GT" },
    { IBUS_DEVICE_DIA,  "DIA" },
    { IBUS_DEVICE_GTF,  "GTF" },
    { IBUS_DEVICE_EWS,  "EWS" },
    { IBUS_DEVICE_CID,  "CID" },
    { IBUS_DEVICE_MFL,  "MFL" },
    { IBUS_DEVICE_IHK,  "IHK" },
    { IBUS_DEVICE_PDC,  "PDC" },
    { IBUS_DEVICE_RAD,  "RAD" },
    { IBUS_DEVICE_DSP,  "DSP" },
    { IBUS_DEVICE_SM0,  "SM0" },
    { IBUS_DEVICE_SDRS, "SDRS" },
    { IBUS_DEVICE_CDCD, "CDCD" },
    { IBUS_DEVICE_NAVE, "NAVE" },
    { IBUS_DEVICE_IKE,  "IKE" },
    { IBUS_DEVICE_SES,  "SES" },
    { IBUS_DEVICE_JNAV, "JNAV" },
    { IBUS_DEVICE_GLO,  "GLO" },
    { IBUS_DEVICE_MID,  "MID" },
    { IBUS_DEVICE_TEL,  "TEL" },
    { IBUS_DEVICE_TCU,  "TCU" },
    { IBUS_DEVICE_LCM,  "LCM" },
    { IBUS_DEVICE_IRIS, "IRIS" },
    { IBUS_DEVICE_ANZV, "ANZV" },
    { IBUS_DEVICE_VM,   "VM" },
    { IBUS_DEVICE_BMBT, "BMBT" },
    { IBUS_DEVICE_LOC,  "LOC" },
};

const char *IBusDeviceName(uint8_t id)
{
    for (size_t i = 0; i < sizeof(ibusDeviceNames)/sizeof(ibusDeviceNames[0]); i++) {
        if (ibusDeviceNames[i].id == id)
            return ibusDeviceNames[i].name;
    }
    return "UNKNOWN";
}

void IBusPrettyPrint(const uint8_t *msg, size_t len, const char *direction)
{
    if (len < 3) {
        LogDebug(LOG_SOURCE_IBUS,
                 "IBus %s: INVALID FRAME (len=%zu)", direction, len);
        return;
    }

    uint8_t src  = msg[0];
    uint8_t size = msg[1];
    uint8_t dst  = msg[2];

    const char *srcName = IBusDeviceName(src);
    const char *dstName = IBusDeviceName(dst);

    // Build payload hex string
    char payload[256];
    size_t p = 0;
    for (size_t i = 3; i < len - 1 && p < sizeof(payload) - 4; i++) {
        p += snprintf(payload + p, sizeof(payload) - p, "%02X ", msg[i]);
    }

    uint8_t chk = msg[len - 1];

    //
    // Column‑aligned output:
    //
    // SRC and DST padded to 4 chars
    // Length printed as 2‑digit decimal
    //
    LogDebug(LOG_SOURCE_IBUS,
             "IBus %s: %-4s → %-4s | Len=%02u | Payload=%-40s | Chk=%02X",
             direction,
             srcName,
             dstName,
             size,
             payload,
             chk);
}
