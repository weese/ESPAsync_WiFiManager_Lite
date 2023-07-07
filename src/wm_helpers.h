#pragma once

#ifndef wm_helpers_h_
#define wm_helpers_h_

#include <SPIFFS.h>

//////////////////////////////////////////////

void resetFunc()
{
  // delay(1000);
  // ESP.restart();
}

//////////////////////////////////////////

uint32_t getChipID()
{
  uint64_t chipId64 = 0;
  for (int i = 0; i < 6; i++)
    chipId64 |= (((uint64_t)ESP.getEfuseMac() >> (40 - (i * 8))) & 0xff) << (i * 8);
  return (uint32_t)(chipId64 & 0xFFFFFF);
}

uint32_t getChipOUI()
{
  uint64_t chipId64 = 0;
  for (int i = 0; i < 6; i++)
    chipId64 |= (((uint64_t)ESP.getEfuseMac() >> (40 - (i * 8))) & 0xff) << (i * 8);
  return (uint32_t)(chipId64 >> 24);
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

#endif // wm_helpers_h_
