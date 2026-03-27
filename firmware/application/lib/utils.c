#include <stdint.h>
#include "utils.h"

static const char UTILS_CHARS_LATIN[] =
    "AAAA\xa1""AACEEEEIIII" /* 00C0-00CF */
    "D\xaf""OOOO\xa2*\xa7UUU\xa3Yp\xa0" /* 00D0-00DF */
    "aaaa\xa4""aaceeeeiiii" /* 00E0-00EF */
    "dnoooo\xa5/\xa9uuu\xa6yby" /* 00F0-00FF */
    "AaAaAaCcCcCcCcDd" /* 0100-010F */
    "DdEeEeEeEeEeGgGg" /* 0110-011F */
    "GgGgHhHhIiIiIiIi" /* 0120-012F */
    "IiJjJjKkkLlLlLlL" /* 0130-013F */
    "lLlNnNnNnnNnOoOo" /* 0140-014F */
    "OoOoRrRrRrSsSsSs" /* 0150-015F */
    "SsTtTtTtUuUuUuUu" /* 0160-016F */
    "UuUuWwYyYZzZzZzF"; /* 0170-017F */

static int8_t BOARD_VERSION = -1;

uint8_t UtilsConvertCmToIn(uint8_t cm)
{
    float impVal = cm / 2.54;
    return (int)(impVal < 0 ? (impVal - 0.5) : (impVal + 0.5));
}

UtilsAbstractDisplayValue_t UtilsDisplayValueInit(char *text, uint8_t status)
{
    UtilsAbstractDisplayValue_t value;
    UtilsStrncpy(value.text, text, UTILS_DISPLAY_TEXT_SIZE);
    value.index = 0;
    value.timeout = 0;
    value.status = status;
    value.length = strlen(text);
    return value;
}

uint8_t UtilsGetBoardVersion()
{
/*    if (BOARD_VERSION == -1) {
        if (BOARD_VERSION_STATUS == BOARD_VERSION_ONE) {
            BOARD_VERSION = BOARD_VERSION_ONE;
        } else {
            BOARD_VERSION = BOARD_VERSION_TWO;
        }
    }*/
    return BOARD_VERSION;
}

uint8_t UtilsGetMinByte(uint8_t *bytes, uint8_t length)
{
    uint8_t minValue = 255;
    uint8_t i;
    for (i = 0; i < length; i++) {
        if (bytes[i] < minValue) {
            minValue = bytes[i];
        }
    }
    return minValue;
}

uint8_t UtilsGetUnicodeByteLength(uint8_t byte)
{
    uint8_t bytesInChar = 1;
    if (byte >> 3 == 30) { // 0xF0 - 0xF4
        bytesInChar = 4;
    } else if (byte >> 4 == 14) { // 0xE0 - 0xEF
        bytesInChar = 3;
    } else if (byte >> 5 == 6) { // 0xC2 - 0xDF
        bytesInChar = 2;
    }
    return bytesInChar;
}

void UtilsNormalizeText(char *string, const char *input, uint16_t max_len)
{
    uint16_t idx = 0;
    uint16_t strIdx = 0;
    uint32_t unicodeChar;

    char *transStr;
    uint8_t transIdx;
    uint8_t transStrLength;

    uint16_t strLength = strlen(input);
    uint8_t bytesInChar = 0;
    uint8_t language = ConfigGetSetting(CONFIG_SETTING_LANGUAGE);


    uint8_t uiMode = ConfigGetUIMode();

    while (idx < strLength && strIdx < (max_len - 1)) {
        uint8_t currentChar = (uint8_t) input[idx];
        unicodeChar = currentChar;

        if (currentChar == '\\') {
            unicodeChar = 0;
            char currentByteBuf[] = {input[idx + 1], input[idx + 2], '\0'};
            uint8_t currentByte = UtilsStrToHex(currentByteBuf);
            bytesInChar = UtilsGetUnicodeByteLength(currentByte);
            uint8_t charsToRead = bytesInChar * 3;
            if ((idx + charsToRead) <= strLength) {
                uint8_t byteIdx = idx;
                while (bytesInChar != 0) {
                    char buf[] = {input[byteIdx + 1], input[byteIdx + 2], '\0'};
                    uint8_t byte = UtilsStrToHex(buf);
                    unicodeChar = unicodeChar << 8 | byte;
                    bytesInChar--;
                    byteIdx = byteIdx + 3;
                }
                idx = idx + charsToRead;
            } else {
                idx = strLength;
            }
        } else if (currentChar > 0x7F) {
            unicodeChar = 0;
            bytesInChar = UtilsGetUnicodeByteLength(currentChar);
            if ((idx + bytesInChar) <= strLength) {
                while (bytesInChar != 0) {
                    uint8_t byte = input[idx];
                    unicodeChar = unicodeChar << 8 | byte;
                    bytesInChar--;
                    idx++;
                }
            } else {
                idx = strLength;
            }
        } else {
            idx++;
        }

        if (unicodeChar >= 0x20 && unicodeChar <= 0x7E) {
            string[strIdx++] = (char) unicodeChar;
        } else if ((uiMode == CONFIG_UI_BMBT)&&(unicodeChar >= 0xA0)&& (unicodeChar <= 0xFC)) {
            string[strIdx++] = (char) unicodeChar;
        } else if (unicodeChar >= 0xC0 && unicodeChar <= 0x017F) {
            string[strIdx++] = UTILS_CHARS_LATIN[unicodeChar - 0xC0];
        } else if (unicodeChar >= 0xC280 && unicodeChar <= 0xC3BF) {
            if (language == CONFIG_SETTING_LANGUAGE_RUSSIAN &&
                unicodeChar >= 0xC380
            ) {
                transStr = UtilsTransliterateExtendedASCIIToASCII(unicodeChar);
                transStrLength = strlen(transStr);

                if ((transStrLength != 0)&&(strIdx+transStrLength<(max_len-1))) {
                    for (transIdx = 0; transIdx < transStrLength; transIdx++) {
                        string[strIdx++] = (char)transStr[transIdx];
                    }
                }
            } else {
                uint32_t extendedChar = (unicodeChar & 0xFF) + ((unicodeChar >> 8) - 0xC2) * 64;
                if (uiMode == CONFIG_UI_BMBT && extendedChar >= 0xA0 && extendedChar <= 0xFC) {
                    string[strIdx++] = (char) extendedChar;
                } else if (extendedChar >= 0xC0 && extendedChar <= 0x017F) {
                    string[strIdx++] = UTILS_CHARS_LATIN[extendedChar - 0xC0];
                }
            }
        } else if (unicodeChar > 0xC3BF) {
            uint8_t transChar = 0;
            if (language == CONFIG_SETTING_LANGUAGE_RUSSIAN) {
                transChar = UtilsConvertCyrillicUnicodeToExtendedASCII(unicodeChar);
            }
            if (transChar != 0) {
                string[strIdx++] = transChar;
            } else {
                uint32_t extendedChar = (unicodeChar & 0xFF) + ((unicodeChar >> 8) - 0xC2) * 64;
                if (uiMode == CONFIG_UI_BMBT && extendedChar >= 0xA0 && extendedChar <= 0xFC) {
                    string[strIdx++] = (char) extendedChar;
                } else if (extendedChar >= 0xC0 && extendedChar <= 0x017F) {
                    string[strIdx++] = UTILS_CHARS_LATIN[extendedChar - 0xC0];
                } else {
                    transStr = UtilsTransliterateUnicodeToASCII(unicodeChar);
                    transStrLength = strlen(transStr);
                    if ((transStrLength != 0)&&(strIdx+transStrLength<(max_len-1))) {
                        for (transIdx = 0; transIdx < transStrLength; transIdx++) {
                            string[strIdx++] = (char)transStr[transIdx];
                        }
                    }
                }
            }
        }
    }
    string[strIdx] = '\0';
}

void UtilsRemoveSubstring(char *string, const char *trash)
{
    uint16_t removeLength = strlen(trash);
    while ((string = strstr(string, trash))) {
        memmove(string, string + removeLength, 1 + strlen(string + removeLength));
    }
}

/*void UtilsReset()
{
    __asm__ volatile("RESET");
}*/

void UtilsSetRPORMode(uint8_t pin, uint16_t mode)
{
    /*if (pin > UTILS_MAX_RPOR_PIN) {
        return;
    }
    uint8_t regNum = 0;
    if (pin > 1) {
        regNum = pin / 2;
    }
    volatile uint16_t *PROG_PIN = GET_RPOR(regNum);
    if ((pin % 2) == 0) {
        uint16_t msb = *PROG_PIN >> 8;
        *PROG_PIN = (msb << 8) + mode;
    } else {
        uint16_t lsb = *PROG_PIN & 0xFF;
        *PROG_PIN = (mode << 8) + lsb;
    }*/
}

void UtilsSetPinMode(uint8_t pin, uint8_t mode)
{
    /*if (pin == UTILS_PIN_TEL_ON) {
        if (UtilsGetBoardVersion() == BOARD_VERSION_ONE) {
            TEL_ON_V1 = mode;
        } else {
            TEL_ON_V2 = mode;
        }
    } else if (pin == UTILS_PIN_TEL_MUTE) {
        if (UtilsGetBoardVersion() == BOARD_VERSION_ONE) {
            TEL_MUTE_V1 = mode;
        } else {
            TEL_MUTE_V2 = mode;
        }
    }*/
}

int8_t UtilsStricmp(const char *string, const char *compare)
{
    int8_t result;
    while(!(result = toupper(*string) - toupper(*compare)) && *string) {
        string++;
        compare++;
    }
    return result;
}

char * UtilsStrncpy(char *dest, const char *src, size_t size)
{
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
    return dest;
}

uint8_t UtilsStrToHex(char *string)
{
    char *ptr;
    return (uint8_t) strtol(string, &ptr, 16);
}

uint8_t UtilsStrToInt(char *string)
{
    char *ptr;
    return (uint8_t) strtol(string, &ptr, 10);
}

char * UtilsTransliterateUnicodeToASCII(uint32_t input)
{
    switch (input) {
        case UTILS_CHAR_LATIN_SMALL_CAPITAL_R:
            return "R";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_IO:
            return "Yo";
            break;
        case UTILS_CHAR_CYRILLIC_UA_CAPITAL_IE:
            return "E";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_I:
        case UTILS_CHAR_CYRILLIC_BY_UA_CAPITAL_I:
        case UTILS_CHAR_CYRILLIC_CAPITAL_YI:
            return "I";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_A:
            return "A";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_BE:
            return "B";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_VE:
            return "V";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_GHE:
            return "G";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_DE:
            return "D";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YE:
            return "Ye";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ZHE:
            return "Zh";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ZE:
            return "Z";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHORT_I:
            return "Y";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_KA:
            return "K";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EL:
            return "L";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EM:
            return "M";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EN:
            return "N";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_O:
            return "O";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_PE:
            return "P";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ER:
            return "R";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ES:
            return "S";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_TE:
            return "T";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_U:
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHORT_U:
            return "U";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EF:
            return "F";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_HA:
            return "Kh";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_TSE:
            return "Ts";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_CHE:
            return "Ch";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHA:
            return "Sh";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SCHA:
            return "Shch";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_HARD_SIGN:
            return "\"";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YERU:
            return "Y";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SOFT_SIGN:
            return "'";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_E:
            return "E";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YU:
            return "Yu";
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YA:
            return "Ya";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_A:
            return "a";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_BE:
            return "b";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_VE:
            return "v";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_GHE:
            return "g";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_DE:
            return "d";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_IE:
        case UTILS_CHAR_CYRILLIC_UA_SMALL_IE:
            return "ye";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ZHE:
            return "zh";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ZE:
            return "z";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_I:
        case UTILS_CHAR_CYRILLIC_BY_UA_SMALL_I:
        case UTILS_CHAR_CYRILLIC_SMALL_YI:
            return "i";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHORT_I:
            return "y";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_KA:
            return "k";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EL:
            return "l";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EM:
            return "m";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EN:
            return "n";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_O:
            return "o";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_PE:
            return "p";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ER:
            return "r";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ES:
            return "s";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_TE:
            return "t";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_U:
        case UTILS_CHAR_CYRILLIC_SMALL_SHORT_U:
            return "u";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EF:
            return "f";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_HA:
            return "kh";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_TSE:
            return "ts";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_CHE:
            return "ch";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHA:
            return "sh";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHCHA:
            return "shch";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_LEFT_HARD_SIGN:
            return "\"";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YERU:
            return "y";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SOFT_SIGN:
            return "'";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YU:
            return "yu";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YA:
            return "ya";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_IO:
            return "yo";
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_E:
            return "e";
            break;
        case UTILS_CHAR_HYPHEN:
            return "-";
            break;
        case UTILS_CHAR_LEFT_SINGLE_QUOTATION_MARK:
        case UTILS_CHAR_RIGHT_SINGLE_QUOTATION_MARK:
            return "'";
            break;
        case UTILS_CHAR_HORIZONTAL_ELLIPSIS:
            return "...";
            break;
        default:
            return "";
            break;
    }
}

char * UtilsTransliterateExtendedASCIIToASCII(uint32_t input)
{
    switch (input) {
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_TILDE:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_RING_ABOVE:
            return "A";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_AE:
            return "Ae";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_C_WITH_CEDILLA:
            return "C";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_DIAERESIS:
            return "E";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_DIAERESIS:
            return "I";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_ETH:
            return "Eth";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_N_WITH_TILDE:
            return "N";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_TILDE:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_STROKE:
            return "O";
            break;
        case UTILS_CHAR_MULTIPLICATION_SIGN:
            return "x";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_DIAERESIS:
            return "U";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_Y_WITH_ACUTE:
            return "Y";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_THORN:
            return "Th";
            break;
        case UTILS_CHAR_LATIN_SMALL_SHARP_S:
            return "ss";
            break;
        case UTILS_CHAR_LATIN_SMALL_A_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_TILDE:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_RING_ABOVE:
            return "a";
            break;
        case UTILS_CHAR_LATIN_SMALL_AE:
            return "ae";
            break;
        case UTILS_CHAR_LATIN_SMALL_C_WITH_CEDILLA:
            return "c";
            break;
        case UTILS_CHAR_LATIN_SMALL_E_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_E_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_E_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_E_WITH_DIAERESIS:
            return "e";
            break;
        case UTILS_CHAR_LATIN_SMALL_I_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_I_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_I_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_I_WITH_DIAERESIS:
            return "i";
            break;
        case UTILS_CHAR_LATIN_SMALL_ETH:
            return "eth";
            break;
        case UTILS_CHAR_LATIN_SMALL_N_WITH_TILDE:
            return "n";
            break;
        case UTILS_CHAR_LATIN_SMALL_O_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_TILDE:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_STROKE:
            return "o";
            break;
        case UTILS_CHAR_DIVISION_SIGN:
            return "%";
            break;
        case UTILS_CHAR_LATIN_SMALL_U_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_U_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_U_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_U_WITH_DIAERESIS:
            return "u";
            break;
        case UTILS_CHAR_LATIN_SMALL_Y_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_Y_WITH_DIAERESIS:
            return "y";
            break;
        case UTILS_CHAR_LATIN_SMALL_THORN:
            return "th";
            break;
        default:
            return "";
            break;
    }
}

uint8_t UtilsConvertCyrillicUnicodeToExtendedASCII(uint32_t input)
{
    switch (input) {
        case UTILS_CHAR_CYRILLIC_CAPITAL_A:
            return 192;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_BE:
            return 193;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_VE:
            return 194;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_GHE:
            return 195;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_DE:
            return 196;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_IO:
        case UTILS_CHAR_CYRILLIC_UA_CAPITAL_IE:
        case UTILS_CHAR_CYRILLIC_CAPITAL_YE:
            return 197;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ZHE:
            return 198;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ZE:
            return 199;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_I:
            return 200;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHORT_I:
            return 201;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_KA:
            return 202;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EL:
            return 203;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EM:
            return 204;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EN:
            return 205;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_O:
            return 206;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_PE:
            return 207;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ER:
            return 208;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ES:
            return 209;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_TE:
            return 210;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_U:
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHORT_U:
            return 211;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EF:
            return 212;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_HA:
            return 213;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_TSE:
            return 214;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_CHE:
            return 215;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHA:
            return 216;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SCHA:
            return 217;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_HARD_SIGN:
            return 218;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YERU:
            return 219;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SOFT_SIGN:
            return 220;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_E:
            return 221;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YU:
            return 222;
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YA:
            return 223;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_A:
            return 224;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_BE:
            return 225;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_VE:
            return 226;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_GHE:
            return 227;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_DE:
            return 228;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_IE:
        case UTILS_CHAR_CYRILLIC_SMALL_IO:
        case UTILS_CHAR_CYRILLIC_UA_SMALL_IE:
            return 229;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ZHE:
            return 230;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ZE:
            return 231;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_I:
            return 232;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHORT_I:
            return 233;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_KA:
            return 234;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EL:
            return 235;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EM:
            return 236;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EN:
            return 237;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_O:
            return 238;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_PE:
            return 239;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ER:
            return 240;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ES:
            return 241;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_TE:
            return 242;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_U:
        case UTILS_CHAR_CYRILLIC_SMALL_SHORT_U:
            return 243;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EF:
            return 244;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_HA:
            return 245;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_TSE:
            return 246;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_CHE:
            return 247;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHA:
            return 248;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHCHA:
            return 249;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_LEFT_HARD_SIGN:
            return 250;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YERU:
            return 251;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SOFT_SIGN:
            return 252;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_E:
            return 253;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YU:
            return 254;
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YA:
            return 255;
            break;
        default:
            return 0;
            break;
    }
}
