#pragma once

#ifndef wm_helpers_h_
#define wm_helpers_h_

#include <SPIFFS.h>
#include "esp_debug_helpers.h"

///////////////////////////////////////////

#ifndef WM_VERSION
  #define WM_VERSION_MAJOR  "1"
  #define WM_VERSION_MINOR  "10"
  #define WM_VERSION_PATCH  "5"

  #define WM_VERSION        "WifiManager v" WM_VERSION_MAJOR "." WM_VERSION_MINOR "." WM_VERSION_PATCH
  #define WM_VERSION_INT    1010005
#endif

//////////////////////////////////////////////

class WMByteArray : public String {
public:
    inline void setLength(int l) { String::setLen(l); }
};

//////////////////////////////////////////////

void resetFunc()
{
  // delay(1000);
  // ESP.restart();
}

//////////////////////////////////////////

uint64_t _getChipID64()
{
  uint64_t chipId64 = 0;
  for (int i = 0; i < 6; i++)
    chipId64 |= (((uint64_t)ESP.getEfuseMac() >> (40 - (i * 8))) & 0xff) << (i * 8);
  return chipId64;
}

inline uint32_t getChipID()
{
  return (uint32_t)(_getChipID64() & 0xFFFFFF);
}

inline uint32_t getChipOUI()
{
  return (uint32_t)(_getChipID64() >> 24);
}

//////////////////////////////////////////

void jsonComma(String &json)
{
  if (json.length())
  {
    char lastChar = json[json.length() - 1];
    if (lastChar != '{' && lastChar != '[')
      json += ',';
  }
}

void jsonAppendValue(String &json, String value)
{
  jsonComma(json);
  json += '"';
  value.replace(FPSTR("\""), FPSTR("\\\""));
  json += value;
  json += '"';
}

void jsonAppendKeyValue(String &json, String const &key, String value, const char *obfuscateValue = NULL)
{
  jsonComma(json);
  json += '"';
  json += key;
  json += FPSTR("\":\"");
  if (obfuscateValue != NULL && !value.isEmpty())
    json += obfuscateValue;
  else
  {
    value.replace(FPSTR("\""), FPSTR("\\\""));
    json += value;
  }
  json += '"';
}

//////////////////////////////////////////

int getRSSIasQuality(int RSSI)
{
  if (RSSI <= -100)
    return 0;
  if (RSSI >= -50)
    return 100;
  return 2 * (RSSI + 100);
}

//////////////////////////////////////////

void listSPIFFSFiles()
{
  Serial.println("List of files in SPIFFS:");

  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file)
  {
    Serial.print("File: ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile();
  }
}

//////////////////////////////////////////

void printStackTrace() {
    esp_backtrace_print(10); // Maximum depth of the backtrace
}

#endif // wm_helpers_h_
