#pragma once

#ifndef wm_config_h_
#define wm_config_h_

#include <string.h>
#include <esp_rom_crc.h>
#include "wm_file.h"
#include "wm_debug.h"

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

    const char * getSSID(const uint8_t& index) const {
      return index < WM_NUM_WIFI_CREDENTIALS ? wifiCreds[index].ssid : "";
    }

    const char * getPW(const uint8_t& index) const {
      return index < WM_NUM_WIFI_CREDENTIALS ? wifiCreds[index].pw : "";
    }

    void resetZero()
    {
        memset(this, 0, sizeof(Configuration));
        strcpy(header, WM_BOARD_TYPE);
    }

    bool isZero() { return checksum == 0; }

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
        checksum = calcChecksum();
        ESP_WML_LOGINFO1(F("WCSum=0x"), String(checksum, HEX));

        bool ok = saveFile((uint8_t *)this, sizeof(Configuration), filename);
        ESP_WML_LOGINFO(ok ? F("OK") : F("failed"));
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
        if (loadFile((uint8_t *)this, sizeof(Configuration), WM_CONFIG_FILENAME) ||
            loadFile((uint8_t *)this, sizeof(Configuration), WM_CONFIG_FILENAME_BACKUP))
        {
            ESP_WML_LOGINFO(F("OK"));
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
} WMConfig;

//////////////////////////////////////////////

#endif // wm_config_h_
