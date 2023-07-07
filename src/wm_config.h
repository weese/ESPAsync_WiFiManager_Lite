#pragma once

#ifndef wm_config_h_
#define wm_config_h_

#include <string.h>
#include <esp_rom_crc.h>
#include "wm_debug.h"

// To be sure no LittleFS for ESP32-C3 for core v1.0.6-
#if (defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2))
// For core v2.0.0+, ESP32-C3 can use LittleFS, SPIFFS or EEPROM
// LittleFS has higher priority than SPIFFS.
// For core v2.0.0+, if not specified any, use better LittleFS
#if !(defined(USE_LITTLEFS) || defined(USE_SPIFFS))
#define USE_LITTLEFS false
#define USE_SPIFFS true
#endif
#elif defined(ARDUINO_ESP32C3_DEV)
// For core v1.0.6-, ESP32-C3 only supporting SPIFFS and EEPROM. To use v2.0.0+ for LittleFS
#if USE_LITTLEFS
#undef USE_LITTLEFS
#define USE_LITTLEFS false
#undef USE_SPIFFS
#define USE_SPIFFS true
#endif
#else
// For core v1.0.6-, if not specified any, use SPIFFS to not forcing user to install LITTLEFS library
#if !(defined(USE_LITTLEFS) || defined(USE_SPIFFS))
#define USE_SPIFFS true
#endif
#endif

#if USE_LITTLEFS
// Use LittleFS
#include "FS.h"

// Check cores/esp32/esp_arduino_version.h and cores/esp32/core_version.h
// #if ( ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2, 0, 0) )  //(ESP_ARDUINO_VERSION_MAJOR >= 2)
#if (defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2))
#warning Using ESP32 Core 1.0.6 or 2.0.0+
// The library has been merged into esp32 core from release 1.0.6
#include <LittleFS.h> // https://github.com/espressif/arduino-esp32/tree/master/libraries/LittleFS

FS *filesystem = &LittleFS;
#define FileFS LittleFS
#define FS_Name "LittleFS"
#else
#warning Using ESP32 Core 1.0.5-. You must install LITTLEFS library
// The library has been merged into esp32 core from release 1.0.6
#include <LITTLEFS.h> // https://github.com/lorol/LITTLEFS

FS *filesystem = &LITTLEFS;
#define FileFS LITTLEFS
#define FS_Name "LittleFS"
#endif

#elif USE_SPIFFS
#include "FS.h"
#include <SPIFFS.h>
FS *filesystem = &SPIFFS;
#define FileFS SPIFFS
#define FS_Name "SPIFFS"
#warning Using SPIFFS in ESPAsync_WiFiManager_Lite.h
#else
#include <EEPROM.h>
#define FS_Name "EEPROM"
#define EEPROM_SIZE 2048
#warning Using EEPROM in ESPAsync_WiFiManager_Lite.h
#endif

//////////////////////////////////////////////

// Use LittleFS/InternalFS for nRF52
#define WM_CONFIG_FILENAME ("/wm_config.dat")
#define WM_CONFIG_FILENAME_BACKUP ("/wm_config.bak")

#define WM_BOARD_TYPE "ESP32_WM"
#define WM_NO_CONFIG "blank"

#define WM_SSID_MAX_LEN 32
// WPA2 passwords can be up to 63 characters long.
#define WM_PASSWORD_MIN_LEN 8
#define WM_PASSWORD_MAX_LEN 64
#define WM_NUM_WIFI_CREDENTIALS 2
#define WM_HEADER_MAX_LEN 16
#define WM_BOARD_NAME_MAX_LEN 24

typedef struct
{
    char ssid[WM_SSID_MAX_LEN];
    char pw[WM_PASSWORD_MAX_LEN];
} WifiCredentials;

extern const struct Configuration defaultConfig;

typedef struct Configuration
{
    char header[WM_HEADER_MAX_LEN];
    WifiCredentials wifiCreds[WM_NUM_WIFI_CREDENTIALS];
    char boardName[WM_BOARD_NAME_MAX_LEN];
    unsigned checksum;

    Configuration() {
        resetZero();
    }

    void resetZero()
    {
        memset(this, 0, sizeof(Configuration));
        strcpy(header, WM_BOARD_TYPE);
    }

    void resetDefault()
    {
        memcpy(this, &defaultConfig, sizeof(Configuration));
        strcpy(header, WM_BOARD_TYPE);
    }

    unsigned calcChecksum() const
    {
        return esp_rom_crc32_le(0, (uint8_t *)this, sizeof(Configuration) - sizeof(checksum));
    }

    bool configValid() const
    {
        unsigned expectedChecksum = calcChecksum();
        ESP_WML_LOGINFO3(F("CCSum=0x"), String(expectedChecksum, HEX), F(",RCSum=0x"), String(checksum, HEX));
        return strcmp(header, WM_BOARD_TYPE) == 0 && checksum == expectedChecksum;
    }

    bool wifiConfigValidPart(size_t i) const
    {
        // If SSID ="blank" or NULL, or PWD length < 8 (as required by standard) => return false
        return strcmp(wifiCreds[i].ssid, WM_NO_CONFIG) &&
               strcmp(wifiCreds[i].pw, WM_NO_CONFIG) &&
               (strlen(wifiCreds[i].ssid) > 0) &&
               (strlen(wifiCreds[i].pw) >= WM_PASSWORD_MIN_LEN);
    }

    bool wifiConfigValid() const
    {
        for (size_t i = 0; i < WM_NUM_WIFI_CREDENTIALS; ++i)
#if REQUIRE_ONE_SET_SSID_PW
            if (wifiConfigValidPart(i))
                return true;
        return false;
#else
            if (!wifiConfigValidPart(i))
                return false;
        return true;
#endif
    }

    //////////////////////////////////////////////

    void saveAs(char const *filename)
    {
        File file = FileFS.open(filename, "w");
        ESP_WML_LOGINFO1(F("SaveCfgFile "), filename);

        checksum = calcChecksum();
        ESP_WML_LOGINFO1(F("WCSum=0x"), String(checksum, HEX));

        if (file)
        {
            file.write((uint8_t *)this, sizeof(Configuration));
            file.close();
            ESP_WML_LOGINFO(F("OK"));
        }
        else
            ESP_WML_LOGINFO(F("failed"));
    }

    void save()
    {
        saveAs(WM_CONFIG_FILENAME);
        saveAs(WM_CONFIG_FILENAME_BACKUP);
    }

    //////////////////////////////////////////////

    void clear()
    {
        resetZero();
        save();
    }

    //////////////////////////////////////////////

    // Return false if init new EEPROM or SPIFFS. No more need trying to connect. Go directly to config mode
    bool load()
    {
        // Format SPIFFS if not yet
        if (FileFS.begin(FORMAT_FS))
        {
            File file = FileFS.open(WM_CONFIG_FILENAME, "r");
            ESP_WML_LOGINFO(F("LoadCfgFile "));

            if (!file)
            {
                ESP_WML_LOGINFO(F("failed"));

                // Trying open redundant config file
                file = FileFS.open(WM_CONFIG_FILENAME_BACKUP, "r");
                ESP_WML_LOGINFO(F("LoadBkUpCfgFile "));
            }

            if (file && file.readBytes((char *)this, sizeof(Configuration)) == sizeof(Configuration))
            {
                ESP_WML_LOGINFO(F("OK"));
                file.close();
                if (configValid() && wifiConfigValid())
                {
                    print();
                    return true;
                }
                else
                    ESP_WML_LOGINFO(F("checksum mismatch"));
            }
            else
                ESP_WML_LOGINFO(F("failed to read"));
        }
        else
            ESP_WML_LOGERROR(F(FS_Name " failed!. Please use another FS or EEPROM."));
        resetDefault();
        return false;
    }

    //////////////////////////////////////////////

    void print() const
    {
        ESP_WML_LOGERROR5(F("Hdr="), header, F(",SSID="), wifiCreds[0].ssid,
                          F(",PW="), wifiCreds[0].pw);
        ESP_WML_LOGERROR3(F("SSID1="), wifiCreds[1].ssid, F(",PW1="), wifiCreds[1].pw);
        ESP_WML_LOGERROR1(F("BName="), boardName);
    }
} ESP_WM_LITE_Configuration;

//////////////////////////////////////////////

#endif // wm_config_h_
