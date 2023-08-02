#pragma once

#ifndef wm_file_h_
#define wm_file_h_

#include <string.h>
#include <esp_rom_crc.h>
#include "wm_debug.h"
#include "wm_helpers.h"

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
#warning Using SPIFFS in wm_file.h
#endif

//////////////////////////////////////////////

bool fileExist(char const *filename)
{
    if (FileFS.begin())
        return FileFS.exists(filename);
    return false;
}

//////////////////////////////////////////////

bool saveFile(uint8_t *buffer, size_t length, char const *filename)
{
    if (FileFS.begin())
    {
        File file = FileFS.open(filename, "w");
        if (file)
        {
            file.write(buffer, length);
            file.close();
            return true;
        }
    }
    return false;
}

bool saveFile(String const &buffer, char const *filename)
{
    return saveFile((uint8_t *)buffer.begin(), buffer.length(), filename);
}

//////////////////////////////////////////////

bool loadFile(uint8_t *buffer, size_t length, char const *filename)
{
    if (FileFS.begin())
    {
        File file = FileFS.open(filename, "r");
        if (file)
        {
            if (file.read(buffer, length) == length)
            {
                file.close();
                return true;
            }
            file.close();
        }
    }
    return false;
}

//////////////////////////////////////////////

bool loadFile(String &buffer, char const *filename)
{
    if (FileFS.begin())
    {
        File file = FileFS.open(filename, "r");
        if (file)
        {
            size_t length = file.size();
            WMByteArray &b = static_cast<WMByteArray &>(buffer);
            b.setLength(length);
            if (file.read((uint8_t *)buffer.begin(), length) == length)
            {
                file.close();
                return true;
            }
            file.close();
        }
    }
    return false;
}

//////////////////////////////////////////////

#endif // wm_file_h_
