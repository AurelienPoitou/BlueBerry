#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "config.h"

#define CONFIG_FILE "config.bin"

uint8_t CONFIG_SETTING_CACHE[CONFIG_SETTING_CACHE_SIZE] = {0};
uint8_t CONFIG_VALUE_CACHE[CONFIG_VALUE_CACHE_SIZE] = {0};

static inline uint8_t ConfigGetByte(uint8_t address) {
    uint8_t value = 0;
    if (address < CONFIG_SETTING_CACHE_SIZE) {
        value = CONFIG_SETTING_CACHE[address];
    }
    if (value == 0x00) {
        FILE *file = fopen(CONFIG_FILE, "rb");
        if (file) {
            fseek(file, address, SEEK_SET);
            fread(&value, sizeof(uint8_t), 1, file);
            fclose(file);
        }
        if (value == 0xFF) {
            value = 0x00;
        }
        if (address < CONFIG_SETTING_CACHE_SIZE) {
            CONFIG_SETTING_CACHE[address] = value;
        }
    }
    return value;
}

static inline void ConfigSetByte(uint8_t address, uint8_t value) {
    if (address < CONFIG_SETTING_CACHE_SIZE) {
        CONFIG_SETTING_CACHE[address] = value;
    }
    FILE *file = fopen(CONFIG_FILE, "r+b");
    if (file) {
        fseek(file, address, SEEK_SET);
        fwrite(&value, sizeof(uint8_t), 1, file);
        fclose(file);
    }
}

uint16_t ConfigGetBC127BootFailures() {
    uint8_t lsb = ConfigGetByte(CONFIG_INFO_BC127_BOOT_FAIL_COUNTER_LSB);
    uint8_t msb = ConfigGetByte(CONFIG_INFO_BC127_BOOT_FAIL_COUNTER_MSB);
    if (lsb == 0xFF && msb == 0xFF) {
        return 0;
    }
    return (msb << 8) + lsb;
}

void ConfigGetBytes(uint8_t address, uint8_t *data, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        data[i] = ConfigGetByte(address);
        address++;
    }
}

uint8_t ConfigGetByteLowerNibble(uint8_t byte) {
    uint8_t value = ConfigGetByte(byte);
    return value & 0x0F;
}

uint8_t ConfigGetByteUpperNibble(uint8_t byte) {
    uint8_t value = ConfigGetByte(byte);
    return (value & 0xF0) >> 4;
}

uint8_t ConfigGetBuildWeek() {
    return ConfigGetByte(CONFIG_BUILD_DATE_ADDRESS_WEEK);
}

uint8_t ConfigGetBuildYear() {
    return ConfigGetByte(CONFIG_BUILD_DATE_ADDRESS_YEAR);
}

uint8_t ConfigGetComfortLock() {
    return ConfigGetByteUpperNibble(CONFIG_SETTING_COMFORT_LOCKS);
}

uint8_t ConfigGetComfortUnlock() {
    return ConfigGetByteLowerNibble(CONFIG_SETTING_COMFORT_LOCKS);
}

uint8_t ConfigGetFirmwareVersionMajor() {
    return ConfigGetByte(CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS);
}

uint8_t ConfigGetFirmwareVersionMinor() {
    return ConfigGetByte(CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS);
}

uint8_t ConfigGetFirmwareVersionPatch() {
    return ConfigGetByte(CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS);
}

void ConfigGetFirmwareVersionString(char *version) {
    snprintf(
        version,
        9,
        "%d.%d.%d",
        ConfigGetFirmwareVersionMajor(),
        ConfigGetFirmwareVersionMinor(),
        ConfigGetFirmwareVersionPatch()
    );
}

uint8_t ConfigGetIKEType() {
    return ConfigGetByteUpperNibble(CONFIG_VEHICLE_TYPE_ADDRESS);
}

uint8_t ConfigGetLightingFeaturesActive() {
    if (ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS) > 0x01 ||
        ConfigGetSetting(CONFIG_SETTING_COMFORT_PARKING_LAMPS) > 0x01) {
        return CONFIG_SETTING_ON;
    }
    return CONFIG_SETTING_OFF;
}

uint8_t ConfigGetLMVariant() {
    return ConfigGetByte(CONFIG_LM_VARIANT_ADDRESS);
}

uint8_t ConfigGetLog(uint8_t system) {
    uint8_t currentSetting = ConfigGetByte(CONFIG_SETTING_LOG_ADDRESS);
    if (currentSetting == 0x00) {
        currentSetting = 0x01;
        ConfigSetByte(CONFIG_SETTING_LOG_ADDRESS, currentSetting);
    }
    return (currentSetting >> system) & 1;
}

uint8_t ConfigGetNavType() {
    return ConfigGetByte(CONFIG_NAV_TYPE_ADDRESS);
}

uint16_t ConfigGetSerialNumber() {
    uint8_t snMSB = ConfigGetByte(CONFIG_SN_ADDRESS_MSB);
    uint8_t snLSB = ConfigGetByte(CONFIG_SN_ADDRESS_LSB);
    return (snMSB << 8) + snLSB;
}

uint8_t ConfigGetSetting(uint8_t setting) {
    uint8_t value = 0x00;
    if (setting >= CONFIG_SETTING_START_ADDRESS &&
        setting <= CONFIG_SETTING_END_ADDRESS) {
        value = ConfigGetByte(setting);
    }
    return value;
}

void ConfigGetString(uint8_t address, char *string, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        string[i] = ConfigGetByte(address);
        address++;
    }
}

uint8_t ConfigGetTelephonyFeaturesActive() {
    if (ConfigGetSetting(CONFIG_SETTING_HFP_ADDRESS) == CONFIG_SETTING_ON ||
        ConfigGetSetting(CONFIG_SETTING_SELF_PLAY_ADDRESS) == CONFIG_SETTING_ON) {
        return CONFIG_SETTING_ON;
    }
    return CONFIG_SETTING_OFF;
}

uint8_t ConfigGetTempDisplay() {
    return ConfigGetByteLowerNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY);
}

uint8_t ConfigGetTempUnit() {
    return ConfigGetByteUpperNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY);
}

uint8_t ConfigGetDistUnit() {
    return ConfigGetByteUpperNibble(CONFIG_SETTING_BMBT_DIST_UNIT_ADDRESS);
}

uint8_t ConfigGetTrapCount(uint8_t trap) {
    return ConfigGetByte(trap);
}

uint8_t ConfigGetTrapLast() {
    return ConfigGetByte(CONFIG_TRAP_LAST_ERR);
}

uint8_t ConfigGetUIMode() {
    return ConfigGetByte(CONFIG_UI_MODE_ADDRESS);
}

uint8_t ConfigGetVehicleType() {
    return ConfigGetByteLowerNibble(CONFIG_VEHICLE_TYPE_ADDRESS);
}

uint8_t ConfigGetValue(uint8_t value) {
    uint8_t data = 0x00;
    if (value >= CONFIG_VALUE_START_ADDRESS &&
        value <= CONFIG_VALUE_END_ADDRESS) {
        data = CONFIG_VALUE_CACHE[value - CONFIG_VALUE_START_ADDRESS];
        if (data == 0x00) {
            data = ConfigGetByte(value);
            CONFIG_VALUE_CACHE[value - CONFIG_VALUE_START_ADDRESS] = data;
        }
    }
    return data;
}

void ConfigGetVehicleIdentity(uint8_t *vin) {
    uint8_t vinAddress[] = CONFIG_VEHICLE_VIN_ADDRESS;
    for (uint8_t i = 0; i < 5; i++) {
        vin[i] = ConfigGetByte(vinAddress[i]);
    }
}

void ConfigSetBC127BootFailures(uint16_t failureCount) {
    ConfigSetByte(CONFIG_INFO_BC127_BOOT_FAIL_COUNTER_MSB, failureCount >> 8);
    ConfigSetByte(CONFIG_INFO_BC127_BOOT_FAIL_COUNTER_LSB, failureCount & 0xFF);
}

void ConfigSetBootloaderMode(uint8_t bootloaderMode) {
    ConfigSetByte(CONFIG_BOOTLOADER_MODE_ADDRESS, bootloaderMode);
}

void ConfigSetBytes(uint8_t address, const uint8_t *data, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        ConfigSetByte(address, data[i]);
        address++;
    }
}

void ConfigSetByteLowerNibble(uint8_t setting, uint8_t value) {
    uint8_t currentValue = ConfigGetByte(setting);
    currentValue &= 0xF0;
    currentValue |= value & 0x0F;
    ConfigSetByte(setting, currentValue);
}

void ConfigSetByteUpperNibble(uint8_t setting, uint8_t value) {
    uint8_t currentValue = ConfigGetByte(setting);
    currentValue &= 0x0F;
    currentValue |= (value << 4) & 0xF0;
    ConfigSetByte(setting, currentValue);
}

void ConfigSetComfortLock(uint8_t comfortLock) {
    ConfigSetByteUpperNibble(CONFIG_SETTING_COMFORT_LOCKS, comfortLock);
}

void ConfigSetComfortUnlock(uint8_t comfortUnlock) {
    ConfigSetByteLowerNibble(CONFIG_SETTING_COMFORT_LOCKS, comfortUnlock);
}

void ConfigSetFirmwareVersion(uint8_t major, uint8_t minor, uint8_t patch) {
    ConfigSetByte(CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS, major);
    ConfigSetByte(CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS, minor);
    ConfigSetByte(CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS, patch);
}

void ConfigSetIKEType(uint8_t ikeType) {
    ConfigSetByteUpperNibble(CONFIG_VEHICLE_TYPE_ADDRESS, ikeType);
}

void ConfigSetLMVariant(uint8_t variant) {
    ConfigSetByte(CONFIG_LM_VARIANT_ADDRESS, variant);
}

void ConfigSetLog(uint8_t system, uint8_t mode) {
    uint8_t currentSetting = ConfigGetByte(CONFIG_SETTING_LOG_ADDRESS);
    uint8_t currentVal = (currentSetting >> system) & 1;
    if (mode != currentVal) {
        currentSetting ^= 1 << system;
    }
    ConfigSetByte(CONFIG_SETTING_LOG_ADDRESS, currentSetting);
}

void ConfigSetNavType(uint8_t type) {
    ConfigSetByte(CONFIG_NAV_TYPE_ADDRESS, type);
}

void ConfigSetSetting(uint8_t setting, uint8_t value) {
    if (setting >= CONFIG_SETTING_START_ADDRESS &&
        setting <= CONFIG_SETTING_END_ADDRESS) {
        ConfigSetByte(setting, value);
    }
}

void ConfigSetString(uint8_t address, char *string, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        ConfigSetByte(address, string[i]);
        address++;
    }
    ConfigSetByte(address, 0);
}

void ConfigSetTempDisplay(uint8_t tempDisplay) {
    ConfigSetByteLowerNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY, tempDisplay);
}

void ConfigSetTempUnit(uint8_t tempUnit) {
    ConfigSetByteUpperNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY, tempUnit);
}

void ConfigSetDistUnit(uint8_t distUnit) {
    ConfigSetByteUpperNibble(CONFIG_SETTING_BMBT_DIST_UNIT_ADDRESS, distUnit);
}

void ConfigSetTrapCount(uint8_t trap, uint8_t count) {
    if (count >= 0xFE) {
        count = 1;
    }
    ConfigSetByte(trap, count);
}

void ConfigSetTrapIncrement(uint8_t trap) {
    uint8_t count = ConfigGetTrapCount(trap);
    ConfigSetTrapCount(trap, count + 1);
    ConfigSetTrapLast(trap);
}

void ConfigSetTrapLast(uint8_t trap) {
    ConfigSetByte(CONFIG_TRAP_LAST_ERR, trap);
}

void ConfigSetUIMode(uint8_t uiMode) {
    ConfigSetByte(CONFIG_UI_MODE_ADDRESS, uiMode);
}

void ConfigSetValue(uint8_t address, uint8_t value) {
    if (address >= CONFIG_VALUE_START_ADDRESS &&
        address <= CONFIG_VALUE_END_ADDRESS) {
        ConfigSetByte(address, value);
    }
}

void ConfigSetVehicleType(uint8_t vehicleType) {
    ConfigSetByteLowerNibble(CONFIG_VEHICLE_TYPE_ADDRESS, vehicleType);
}

void ConfigSetVehicleIdentity(uint8_t *vin) {
    uint8_t vinAddress[] = CONFIG_VEHICLE_VIN_ADDRESS;
    for (uint8_t i = 0; i < 5; i++) {
        ConfigSetByte(vinAddress[i], vin[i]);
    }
}
