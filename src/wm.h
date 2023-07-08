/****************************************************************************************************************************
  WiFiManager.h
  For ESP32 boards

  WiFiManager (https://github.com/khoih-prog/WiFiManager) is a library
  for the ESP32 boards to enable store Credentials in EEPROM/SPIFFS/LittleFS for easy
  configuration/reconfiguration and autoconnect/autoreconnect of WiFi and other services without Hardcoding.

  Built by Khoi Hoang https://github.com/khoih-prog/WiFiManager
  Licensed under MIT license

  Version: 1.10.5

  Version Modified By   Date        Comments
  ------- -----------  ----------   -----------
  1.0.0   K Hoang      09/02/2021  Initial coding for ESP32/ESP8266
  1.1.0   K Hoang      12/02/2021  Add support to new ESP32-S2
  1.2.0   K Hoang      22/02/2021  Add customs HTML header feature. Fix bug.
  1.3.0   K Hoang      12/04/2021  Fix invalid "blank" Config Data treated as Valid. Fix EEPROM_SIZE bug
  1.4.0   K Hoang      21/04/2021  Add support to new ESP32-C3 using SPIFFS or EEPROM
  1.5.0   Michael H    24/04/2021  Enable scan of WiFi networks for selection in Configuration Portal
  1.5.1   K Hoang      10/10/2021  Update `platform.ini` and `library.json`
  1.6.0   K Hoang      26/11/2021  Auto detect ESP32 core and use either built-in LittleFS or LITTLEFS library. Fix bug.
  1.7.0   K Hoang      09/01/2022  Fix the blocking issue in loop() with configurable WIFI_RECON_INTERVAL
  1.8.0   K Hoang      10/02/2022  Add support to new ESP32-S3
  1.8.1   K Hoang      11/02/2022  Add LittleFS support to ESP32-C3. Use core LittleFS instead of Lorol's LITTLEFS for v2.0.0+
  1.8.2   K Hoang      21/02/2022  Optional Board_Name in Menu. Optimize code by using passing by reference
  1.9.0   K Hoang      09/09/2022  Fix ESP32 chipID and add getChipOUI()
  1.9.1   K Hoang      28/12/2022  Add Captive Portal using AsyncDNSServer
  1.10.1  K Hoang      12/01/2023  Added public methods to load and save dynamic data. Bump up to v1.10.1
  1.10.2  K Hoang      15/01/2023  Add Config Portal scaling support to mobile devices
  1.10.3  K Hoang      19/01/2023  Fix compiler error if EEPROM is used
  1.10.5  K Hoang      29/01/2023  Using PROGMEM for strings. Sync with ESP_WiFiManager_Lite v1.10.5
 *****************************************************************************************************************************/

#pragma once

#ifndef ESPAsync_WiFiManager_Lite_h
#define ESPAsync_WiFiManager_Lite_h

///////////////////////////////////////////

#ifndef ESP32
  #error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#elif ( ARDUINO_ESP32S2_DEV || ARDUINO_FEATHERS2 || ARDUINO_ESP32S2_THING_PLUS || ARDUINO_MICROS2 || \
        ARDUINO_METRO_ESP32S2 || ARDUINO_MAGTAG29_ESP32S2 || ARDUINO_FUNHOUSE_ESP32S2 || \
        ARDUINO_ADAFRUIT_FEATHER_ESP32S2_NOPSRAM )
  #warning Using ESP32_S2.
  #define USING_ESP32_S2        true
#elif ( ARDUINO_ESP32C3_DEV )
  #if ( defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2) )
    #warning Using ESP32_C3 using core v2.0.0+. Either LittleFS, SPIFFS or EEPROM OK.
  #else
    #warning Using ESP32_C3 using core v1.0.6-. To follow library instructions to install esp32-c3 core. Only SPIFFS and EEPROM OK.
  #endif

  #warning You have to select Flash size 2MB and Minimal APP (1.3MB + 700KB) for some boards
  #define USING_ESP32_C3        true
#elif ( defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32_S3_BOX) || defined(ARDUINO_TINYS3) || \
        defined(ARDUINO_PROS3) || defined(ARDUINO_FEATHERS3) )
  #warning Using ESP32_S3. To install esp32-s3-support branch if using core v2.0.2-.
  #define USING_ESP32_S3        true
#endif

///////////////////////////////////////////

#ifndef ESP_ASYNC_WIFI_MANAGER_LITE_VERSION
  #define ESP_ASYNC_WIFI_MANAGER_LITE_VERSION             "WiFiManager v1.10.5"

  #define ESP_ASYNC_WIFI_MANAGER_LITE_VERSION_MAJOR       1
  #define ESP_ASYNC_WIFI_MANAGER_LITE_VERSION_MINOR       10
  #define ESP_ASYNC_WIFI_MANAGER_LITE_VERSION_PATCH       5

  #define ESP_ASYNC_WIFI_MANAGER_LITE_VERSION_INT         1010005
#endif

///////////////////////////////////////////

#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

///////////////////////////////////////////

#define HTTP_PORT     80
#define DNS_PORT      53
#define FORMAT_FS     false

///////////////////////////////////////////

#include <ESPAsyncDNSServer.h>
#include <memory>
#undef min
#undef max
#include <algorithm>

//KH, for ESP32
#include <esp_wifi.h>

uint32_t getChipID();
uint32_t getChipOUI();

#if defined(ESP_getChipId)
  #undef ESP_getChipId
#endif

#if defined(ESP_getChipOUI)
  #undef ESP_getChipOUI
#endif

#define ESP_getChipId()   getChipID()
#define ESP_getChipOUI()  getChipOUI()

#include "wm_debug.h"
#include "wm_helpers.h"
#include "wm_config.h"

//////////////////////////////////////////////

// New from v1.3.0
// KH, Some minor simplification
#if !defined(MAX_SSID_IN_LIST)
  #define MAX_SSID_IN_LIST     10
#elif (MAX_SSID_IN_LIST < 2)
  #warning Parameter MAX_SSID_IN_LIST defined must be >= 2 - Reset to 10
  #undef MAX_SSID_IN_LIST
  #define MAX_SSID_IN_LIST      10
#elif (MAX_SSID_IN_LIST > 15)
  #warning Parameter MAX_SSID_IN_LIST defined must be <= 15 - Reset to 10
  #undef MAX_SSID_IN_LIST
  #define MAX_SSID_IN_LIST      10
#endif

///////// NEW for DRD /////////////
// These defines must be put before #include <ESP_DoubleResetDetector.h>
// to select where to store DoubleResetDetector's variable.
// For ESP32, You must select one to be true (EEPROM or SPIFFS/LittleFS)
// For ESP8266, You must select one to be true (RTC, EEPROM or SPIFFS/LittleFS)
// Otherwise, library will use default EEPROM storage

#if USE_LITTLEFS
  #define ESP_MRD_USE_LITTLEFS    true
  #define ESP_MRD_USE_SPIFFS      false
  #define ESP_MRD_USE_EEPROM      false
#elif USE_SPIFFS
  #define ESP_MRD_USE_LITTLEFS    false
  #define ESP_MRD_USE_SPIFFS      true
  #define ESP_MRD_USE_EEPROM      false
#else
  #define ESP_MRD_USE_LITTLEFS    false
  #define ESP_MRD_USE_SPIFFS      false
  #define ESP_MRD_USE_EEPROM      true
#endif

// Number of seconds after reset during which a
// subsequent reset will be considered a double reset.
#define RD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define RD_ADDRESS 0

#include <ESP_MultiResetDetector.h>      //https://github.com/khoih-prog/ESP_DoubleResetDetector

//DoubleResetDetector rd(RD_TIMEOUT, RD_ADDRESS);
MultiResetDetector* rd;

///////////////////////////////////////////

const char PASS_OBFUSCATE_STRING[]        PROGMEM = "*******";
const char FERMI_CLOUD_USERCODE_URL[]     PROGMEM = "http://fermicloud.spdns.de/auth/realms/fermi-cloud/protocol/openid-connect/auth/device";
const char FERMI_CLOUD_USERCODE_PAYLOAD[] PROGMEM = "client_id=fermi-device";

#define MIN_WIFI_CHANNEL        1
#define MAX_WIFI_CHANNEL        11    // Channel 13 is flaky, because of bad number 13 ;-)

///////////////////////////////////////////

extern bool LOAD_DEFAULT_CONFIG_DATA;
#define WM_CONFIG_PORTAL_FILENAME ("/wm_cp.dat")
#define WM_CONFIG_PORTAL_FILENAME_BACKUP ("/wm_cp.bak")


//////////////////////////////////////////

//KH Add repeatedly used const

const char WM_HTTP_HEAD_TEXT_HTML[]  PROGMEM = "text/html";
const char WM_HTTP_HEAD_TEXT_PLAIN[] PROGMEM = "text/plain";
const char WM_HTTP_HEAD_TEXT_JSON[]  PROGMEM = "text/json";

const char WM_HTTP_CACHE_CONTROL[]   PROGMEM = "Cache-Control";
const char WM_HTTP_NO_STORE[]        PROGMEM = "no-cache, no-store, must-revalidate";
const char WM_HTTP_PRAGMA[]          PROGMEM = "Pragma";
const char WM_HTTP_NO_CACHE[]        PROGMEM = "no-cache";
const char WM_HTTP_EXPIRES[]         PROGMEM = "Expires";
const char WM_HTTP_CORS[]            PROGMEM = "Access-Control-Allow-Origin";
const char WM_HTTP_CORS_ALLOW_ALL[]  PROGMEM = "*";

//////////////////////////////////////////

class WiFiManager
{
  public:

    WiFiManager() {}

    ~WiFiManager() {
      delete dnsServer;
      delete server;
      free(indices);
    }

    void connectWiFi(const char* ssid, const char* pass) {
      ESP_WML_LOGINFO1(F("Con2:"), ssid);
      WiFi.mode(WIFI_STA);

      if (static_IP != IPAddress(0, 0, 0, 0)) {
        ESP_WML_LOGINFO(F("UseStatIP"));
        WiFi.config(static_IP, static_GW, static_SN, static_DNS1, static_DNS2);
      }

      setHostname();

      if (WiFi.status() != WL_CONNECTED)
      {
        if (pass && strlen(pass))
          WiFi.begin(ssid, pass);
        else
          WiFi.begin(ssid);
      }

      while (WiFi.status() != WL_CONNECTED)
        delay(500);

      ESP_WML_LOGINFO(F("Conn2WiFi"));
      displayWiFiData();
    }

    //////////////////////////////////////////

    void begin(const char* ssid,
               const char* pass )
    {
      ESP_WML_LOGERROR(F("conW"));
      connectWiFi(ssid, pass);
    }

    //////////////////////////////////////////

#if !defined(USE_LED_BUILTIN)
  #define USE_LED_BUILTIN     true      // use builtin LED to show configuration mode
#endif

// For ESP32
#ifndef LED_BUILTIN
  #define LED_BUILTIN       2         // Pin D2 mapped to pin GPIO2/ADC12 of ESP32, control on-board LED
#endif

///////////////////////////////////////////

#if !defined(REQUIRE_ONE_SET_SSID_PW)
  #define REQUIRE_ONE_SET_SSID_PW     false
#endif

#define PASSWORD_MIN_LEN        8

    //////////////////////////////////////////

    void begin(const char *iHostname = "", wifi_mode_t mode = WIFI_STA)
    {
#define TIMEOUT_CONNECT_WIFI      30000

#if USE_LED_BUILTIN
      // Turn OFF
      pinMode(LED_BUILTIN, OUTPUT);
      digitalWrite(LED_BUILTIN, LOW);
#endif

      //// New MRD ////
      rd = new MultiResetDetector(RD_TIMEOUT, RD_ADDRESS);

      bool noConfigPortal = true;

      if (rd->detectMultiReset()) {
        ESP_WML_LOGINFO(F("Multi Reset Detected"));
        noConfigPortal = false;
      }

      if (LOAD_DEFAULT_CONFIG_DATA) {
        ESP_WML_LOGDEBUG(F("======= Start Default Config Data ======="));
        defaultConfig.print();
      }

      if (iHostname[0] == 0) {
        String _hostname = "ESP_" + String(ESP_getChipId(), HEX);
        _hostname.toUpperCase();
        getRFC952_hostname(_hostname.c_str());
      }
      else
        // Prepare and store the hostname only not NULL
        getRFC952_hostname(iHostname);

      ESP_WML_LOGINFO1(F("Hostname="), RFC952_hostname);

      hadConfigData = getConfigData();

      isForcedConfigPortal = isForcedCP();

      //  noConfigPortal when getConfigData() OK and no MRD/DRD'ed
      if (hadConfigData && noConfigPortal && (!isForcedConfigPortal) )
      {
        hadConfigData = true;

        ESP_WML_LOGDEBUG(noConfigPortal ? F("bg: noConfigPortal = true") : F("bg: noConfigPortal = false"));

        for (uint16_t i = 0; i < WM_NUM_WIFI_CREDENTIALS; i++)
          if (ESP_WM_LITE_config.wifiConfigValidPart(i)) {
            ESP_WML_LOGDEBUG5(F("bg: addAP : index="), i, F(", SSID="), ESP_WM_LITE_config.wifiCreds[i].ssid, F(", PWD="),
                              ESP_WM_LITE_config.wifiCreds[i].pw);
            wifiMulti.addAP(ESP_WM_LITE_config.wifiCreds[i].ssid, ESP_WM_LITE_config.wifiCreds[i].pw);
          } else
            ESP_WML_LOGWARN3(F("bg: Ignore invalid WiFi PWD : index="), i, F(", PWD="), ESP_WM_LITE_config.wifiCreds[i].pw);

        if (connectMultiWiFi() == WL_CONNECTED) {
          ESP_WML_LOGINFO(F("bg: WiFi OK."));
        } else {
          ESP_WML_LOGINFO(F("bg: Fail2connect WiFi"));
          // failed to connect to WiFi, will start configuration mode
          startConfigurationMode();
        }
        startConfigurationMode();
      } else {
        ESP_WML_LOGDEBUG(isForcedConfigPortal ? F("bg: isForcedConfigPortal = true") : F("bg: isForcedConfigPortal = false"));

        // If not persistent => clear the flag so that after reset. no more CP, even CP not entered and saved
        if (persForcedConfigPortal) {
          ESP_WML_LOGINFO1(F("bg:Stay forever in CP:"),
                           isForcedConfigPortal ? F("Forced-Persistent") : (noConfigPortal ? F("No ConfigDat") : F("MRD")));
        } else {
          ESP_WML_LOGINFO1(F("bg:Stay forever in CP:"),
                           isForcedConfigPortal ? F("Forced-non-Persistent") : (noConfigPortal ? F("No ConfigDat") : F("MRD")));
          clearForcedCP();
        }

        hadConfigData = isForcedConfigPortal ? true : (noConfigPortal ? false : true);

        // failed to connect to WiFi, will start configuration mode
        startConfigurationMode();
      }
    }

    //////////////////////////////////////////

#ifndef TIMEOUT_RECONNECT_WIFI
  #define TIMEOUT_RECONNECT_WIFI   10000L
#else
    // Force range of user-defined TIMEOUT_RECONNECT_WIFI between 10-60s
  #if (TIMEOUT_RECONNECT_WIFI < 10000L)
    #warning TIMEOUT_RECONNECT_WIFI too low. Reseting to 10000
    #undef TIMEOUT_RECONNECT_WIFI
    #define TIMEOUT_RECONNECT_WIFI   10000L
  #elif (TIMEOUT_RECONNECT_WIFI > 60000L)
    #warning TIMEOUT_RECONNECT_WIFI too high. Reseting to 60000
    #undef TIMEOUT_RECONNECT_WIFI
    #define TIMEOUT_RECONNECT_WIFI   60000L
  #endif
#endif

#ifndef RESET_IF_CONFIG_TIMEOUT
  #define RESET_IF_CONFIG_TIMEOUT   true
#endif

#ifndef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
  #define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET          10
#else
  // Force range of user-defined TIMES_BEFORE_RESET between 2-100
  #if (CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET < 2)
    #warning CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET too low. Reseting to 2
    #undef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
    #define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET   2
  #elif (CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET > 100)
    #warning CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET too high. Reseting to 100
    #undef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
    #define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET   100
  #endif
#endif

    //////////////////////////////////////////

#if !defined(WIFI_RECON_INTERVAL)
  #define WIFI_RECON_INTERVAL       0         // default 0s between reconnecting WiFi
#else
  #if (WIFI_RECON_INTERVAL < 0)
    #define WIFI_RECON_INTERVAL     0
  #elif  (WIFI_RECON_INTERVAL > 600000)
    #define WIFI_RECON_INTERVAL     600000    // Max 10min
  #endif
#endif

    //////////////////////////////////////////

    void run()
    {
      static int retryTimes = 0;
      static bool wifiDisconnectedOnce = false;

      // Lost connection in running. Give chance to reconfig.
      // Check WiFi status every 5s and update status
      // Check twice to be sure wifi disconnected is real
      static unsigned long checkstatus_timeout = 0;
#define WIFI_STATUS_CHECK_INTERVAL    5000L

      static uint32_t curMillis;

      curMillis = millis();

      // Call the double reset detector loop method every so often,
      // so that it can recognise when the timeout expires.
      // You can also call rd.stop() when you wish to no longer
      // consider the next reset as a double reset.
      rd->loop();

      if ( !configuration_mode && (curMillis > checkstatus_timeout) )
      {
        if (WiFi.status() == WL_CONNECTED)
          wifi_connected = true;
        else
        {
          if (wifiDisconnectedOnce) {
            wifi_connected = false;
            ESP_WML_LOGERROR(F("r:Check&WLost"));
          }
          wifiDisconnectedOnce = !wifiDisconnectedOnce;
        }

        checkstatus_timeout = curMillis + WIFI_STATUS_CHECK_INTERVAL;
      }

      // Lost connection in running. Give chance to reconfig.
      if ( WiFi.status() != WL_CONNECTED )
      {
        // If configTimeout but user hasn't connected to configWeb => try to reconnect WiFi
        // But if user has connected to configWeb, stay there until done, then reset hardware
        if ( configuration_mode && ( configTimeout == 0 ||  millis() < configTimeout ) )
        {
          retryTimes = 0;
          // Fix ESP32-S2 issue with WebServer (https://github.com/espressif/arduino-esp32/issues/4348)
          if ( String(ARDUINO_BOARD) == "ESP32S2_DEV" )
            delay(1);
          return;
        }

// #if RESET_IF_CONFIG_TIMEOUT

//         // If we're here but still in configuration_mode, permit running TIMES_BEFORE_RESET times before reset hardware
//         // to permit user another chance to config.
//         if ( configuration_mode && (configTimeout != 0) ) {
//           ESP_WML_LOGDEBUG(F("r:Check RESET_IF_CONFIG_TIMEOUT"));

//           if (++retryTimes <= CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET) {
//             ESP_WML_LOGINFO1(F("run: WiFi lost, configTimeout. Connect WiFi. Retry#:"), retryTimes);
//           } else
//             resetFunc();
//         }

// #endif

        // Not in config mode, try reconnecting before forcing to config mode
        if ( WiFi.status() != WL_CONNECTED ) {
#if (WIFI_RECON_INTERVAL > 0)

          static uint32_t lastMillis = 0;

          if ( (lastMillis == 0) || (curMillis - lastMillis) > WIFI_RECON_INTERVAL ) {
            lastMillis = curMillis;

            ESP_WML_LOGERROR(F("r:WLost.ReconW"));

            if (connectMultiWiFi() == WL_CONNECTED)
            {
#if USE_LED_BUILTIN
              // turn the LED_BUILTIN OFF to tell us we exit configuration mode.
              digitalWrite(LED_BUILTIN, LOW);
#endif
              ESP_WML_LOGINFO(F("run: WiFi reconnected"));
            }
          }

#else
          ESP_WML_LOGINFO(F("run: WiFi lost. Reconnect WiFi"));

          if (connectMultiWiFi() == WL_CONNECTED) {
#if USE_LED_BUILTIN
            // turn the LED_BUILTIN OFF to tell us we exit configuration mode.
            digitalWrite(LED_BUILTIN, LOW);
#endif
            ESP_WML_LOGINFO(F("run: WiFi reconnected"));
          }

#endif
        }

        //ESP_WML_LOGINFO(F("run: Lost connection => configMode"));
        //startConfigurationMode();
      }
      else if (configuration_mode)
      {
        // WiFi is connected and we are in configuration_mode
        configuration_mode = false;
        ESP_WML_LOGINFO(F("run: got WiFi back"));

#if USE_LED_BUILTIN
        // turn the LED_BUILTIN OFF to tell us we exit configuration mode.
        digitalWrite(LED_BUILTIN, LOW);
#endif

        if (dnsServer) {
          dnsServer->stop();
          delete dnsServer;
          dnsServer = nullptr;
        }

        if (server) {
          server->end();
          delete server;
          server = nullptr;
        }
      }
    }

    //////////////////////////////////////////////

    bool getWiFiStatus()      { return wifi_connected; }
    bool isConfigDataValid()  { return hadConfigData; }
    bool isConfigMode()       { return configuration_mode; }
    String localIP()          { return WiFi.localIP().toString(); }

    void setMinimumSignalQuality(const int& quality) { _minimumQuality = quality; }
    void setRemoveDuplicateAPs(const bool& removeDuplicates) { _removeDuplicateAPs = removeDuplicates; }
    void setHostname() { if (RFC952_hostname[0] != 0) WiFi.setHostname(RFC952_hostname); }
    void setConfigPortalIP(const IPAddress& portalIP = IPAddress(192, 168, 4, 1)) { portal_apIP = portalIP; }
    bool getConfigData()      { return ESP_WM_LITE_config.load(); }
    void clearConfigData()    { ESP_WM_LITE_config.clear(); }
    void saveConfigData()     { ESP_WM_LITE_config.save(); }
    void saveAllConfigData()  { saveConfigData(); }

    void setConfigPortal(const String& ssid = "", const String& pass = "")
    {
      portal_ssid = ssid;
      portal_pass = pass;
    }

    //////////////////////////////////////////////

    int setConfigPortalChannel(const int& channel = 1)
    {
      // If channel < MIN_WIFI_CHANNEL - 1 or channel > MAX_WIFI_CHANNEL => channel = 1
      // If channel == 0 => will use random channel from MIN_WIFI_CHANNEL to MAX_WIFI_CHANNEL
      // If (MIN_WIFI_CHANNEL <= channel <= MAX_WIFI_CHANNEL) => use it
      if ( (channel < MIN_WIFI_CHANNEL - 1) || (channel > MAX_WIFI_CHANNEL) )
        WiFiAPChannel = 1;
      else if ( (channel >= MIN_WIFI_CHANNEL - 1) && (channel <= MAX_WIFI_CHANNEL) )
        WiFiAPChannel = channel;

      return WiFiAPChannel;
    }

    //////////////////////////////////////////////

    void setSTAStaticIPConfig(const IPAddress& ip, const IPAddress& gw,
                              const IPAddress& sn = IPAddress(255, 255, 255, 0),
                              const IPAddress& dns_address_1 = IPAddress(0, 0, 0, 0),
                              const IPAddress& dns_address_2 = IPAddress(0, 0, 0, 0))
    {
      static_IP     = ip;
      static_GW     = gw;
      static_SN     = sn;

      // Default to local GW
      if (dns_address_1 == IPAddress(0, 0, 0, 0))
        static_DNS1   = gw;
      else
        static_DNS1   = dns_address_1;

      // Default to Google DNS (8, 8, 8, 8)
      if (dns_address_2 == IPAddress(0, 0, 0, 0))
        static_DNS2   = IPAddress(8, 8, 8, 8);
      else
        static_DNS2   = dns_address_2;
    }

    //////////////////////////////////////////////

    void assertConfig() { if (!hadConfigData) getConfigData(); }

    String getWiFiSSID(const uint8_t& index) {
      if (index >= WM_NUM_WIFI_CREDENTIALS)
        return "";

      assertConfig();
      return ESP_WM_LITE_config.wifiCreds[index].ssid;
    }

    String getWiFiPW(const uint8_t& index) {
      if (index >= WM_NUM_WIFI_CREDENTIALS)
        return "";

      assertConfig();
      return ESP_WM_LITE_config.wifiCreds[index].pw;
    }

    String getBoardName() {
      assertConfig();
      return ESP_WM_LITE_config.boardName;
    }

    //////////////////////////////////////////////

    ESP_WM_LITE_Configuration* getFullConfigData(ESP_WM_LITE_Configuration *configData) {
      assertConfig();

      // Check if NULL pointer
      if (configData)
        memcpy(configData, &ESP_WM_LITE_config, sizeof(ESP_WM_LITE_Configuration));

      return configData;
    }

    //////////////////////////////////////////////

    // Forced CP => Flag = 0xBEEFBEEF. Else => No forced CP
    // Flag to be stored at (EEPROM_START + DRD_FLAG_DATA_SIZE + CONFIG_DATA_SIZE)
    // to avoid corruption to current data
    //#define FORCED_CONFIG_PORTAL_FLAG_DATA              ( (uint32_t) 0xDEADBEEF)
    //#define FORCED_PERS_CONFIG_PORTAL_FLAG_DATA         ( (uint32_t) 0xBEEFDEAD)

    const uint32_t FORCED_CONFIG_PORTAL_FLAG_DATA       = 0xDEADBEEF;
    const uint32_t FORCED_PERS_CONFIG_PORTAL_FLAG_DATA  = 0xBEEFDEAD;

#define FORCED_CONFIG_PORTAL_FLAG_DATA_SIZE     4

    void resetAndEnterConfigPortal()
    {
      persForcedConfigPortal = false;

      setForcedCP(false);

      // Delay then reset the ESP8266 after save data
      resetFunc();
    }

    //////////////////////////////////////////////

    // This will keep CP forever, until you successfully enter CP, and Save data to clear the flag.
    void resetAndEnterConfigPortalPersistent()
    {
      persForcedConfigPortal = true;
      setForcedCP(true);
      // Delay then reset the ESP8266 after save data
      resetFunc();
    }

    //////////////////////////////////////

    // New from v1.2.0, for configure CORS Header, default to WM_HTTP_CORS_ALLOW_ALL = "*"

    //////////////////////////////////////

#if USING_CORS_FEATURE
    void setCORSHeader(PGM_P CORSHeaders = NULL)
    {
      _CORS_Header = CORSHeaders;
      ESP_WML_LOGDEBUG1(F("Set CORS Header to : "), _CORS_Header);
    }

    //////////////////////////////////////

    PGM_P getCORSHeader()
    {
      ESP_WML_LOGDEBUG1(F("Get CORS Header = "), _CORS_Header);
      return _CORS_Header;
    }
#endif

    //////////////////////////////////////


  private:
    String ipAddress = "0.0.0.0";

    WiFiMulti wifiMulti;
    AsyncWebServer *server = nullptr;
    AsyncDNSServer *dnsServer = nullptr;

    bool configuration_mode = false;

    unsigned long configTimeout;
    bool hadConfigData = false;
    bool hadDynamicData = false;

    bool isForcedConfigPortal   = false;
    bool persForcedConfigPortal = false;

    ESP_WM_LITE_Configuration ESP_WM_LITE_config;

    uint16_t totalDataSize = 0;

    String macAddress = "";
    bool wifi_connected = false;

    IPAddress portal_apIP = IPAddress(192, 168, 4, 1);
    int WiFiAPChannel = 10;

    String portal_ssid = "";
    String portal_pass = "";

    String deviceCode = "";
    String userCode = "";

    IPAddress static_IP   = IPAddress(0, 0, 0, 0);
    IPAddress static_GW   = IPAddress(0, 0, 0, 0);
    IPAddress static_SN   = IPAddress(255, 255, 255, 0);
    IPAddress static_DNS1 = IPAddress(0, 0, 0, 0);
    IPAddress static_DNS2 = IPAddress(0, 0, 0, 0);

#if USING_CORS_FEATURE
    PGM_P _CORS_Header = WM_HTTP_CORS_ALLOW_ALL;   // "*";
#endif

    int WiFiNetworksFound = 0;    // Number of SSIDs found by WiFi scan, including low quality and duplicates
    int *indices;                 // WiFi network data, filled by scan (SSID, BSSID)
    String ListOfSSIDs = "";      // List of SSIDs found by scan, in HTML <option> format

    //////////////////////////////////////

#define RFC952_HOSTNAME_MAXLEN      24u

    char RFC952_hostname[RFC952_HOSTNAME_MAXLEN + 1];

    char* getRFC952_hostname(const char* iHostname)
    {
      memset(RFC952_hostname, 0, sizeof(RFC952_hostname));
      size_t len = std::min(RFC952_HOSTNAME_MAXLEN, strlen(iHostname));
      size_t j = 0;
      for (size_t i = 0; i < len - 1; i++)
        if (isalnum(iHostname[i]) || iHostname[i] == '-')
          RFC952_hostname[j++] = iHostname[i];

      // no '-' as last char
      if (isalnum(iHostname[len - 1]) || (iHostname[len - 1] != '-'))
        RFC952_hostname[j] = iHostname[len - 1];

      return RFC952_hostname;
    }

    //////////////////////////////////////

    void displayWiFiData()
    {
      ESP_WML_LOGERROR3(F("SSID="), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
      ESP_WML_LOGERROR1(F("IP="), WiFi.localIP() );
    }

    //////////////////////////////////////

    bool isWiFiConfigValid()
    {
      if (!ESP_WM_LITE_config.wifiConfigValid()) {
        // If SSID, PW ="blank" or NULL, set the flag
        ESP_WML_LOGERROR(F("Invalid Stored WiFi Config Data"));

        // Nullify the invalid data to avoid displaying garbage
        ESP_WM_LITE_config.resetZero();

        hadConfigData = false;

        return false;
      }

      return true;
    }

    //////////////////////////////////////////////

    void saveForcedCP(const uint32_t& value)
    {
      File file = FileFS.open(WM_CONFIG_PORTAL_FILENAME, "w");

      ESP_WML_LOGINFO(F("SaveCPFile "));

      if (file) {
        file.write((uint8_t*) &value, sizeof(value));
        file.close();
        ESP_WML_LOGINFO(F("OK"));
      } else
        ESP_WML_LOGINFO(F("failed"));

      // Trying open redundant CP file
      file = FileFS.open(WM_CONFIG_PORTAL_FILENAME_BACKUP, "w");

      ESP_WML_LOGINFO(F("SaveBkUpCPFile "));

      if (file) {
        file.write((uint8_t *) &value, sizeof(value));
        file.close();
        ESP_WML_LOGINFO(F("OK"));
      } else
        ESP_WML_LOGINFO(F("failed"));
    }

    //////////////////////////////////////////////

    void setForcedCP(const bool& isPersistent)
    {
      uint32_t readForcedConfigPortalFlag = isPersistent ? FORCED_PERS_CONFIG_PORTAL_FLAG_DATA :
                                            FORCED_CONFIG_PORTAL_FLAG_DATA;
      ESP_WML_LOGDEBUG(isPersistent ? F("setForcedCP Persistent") : F("setForcedCP non-Persistent"));
      saveForcedCP(readForcedConfigPortalFlag);
    }

    //////////////////////////////////////////////

    void clearForcedCP()
    {
      uint32_t readForcedConfigPortalFlag = 0;
      ESP_WML_LOGDEBUG(F("clearForcedCP"));
      saveForcedCP(readForcedConfigPortalFlag);
    }

    //////////////////////////////////////////////

    bool isForcedCP()
    {
      uint32_t readForcedConfigPortalFlag;
      ESP_WML_LOGDEBUG(F("Check if isForcedCP"));

      File file = FileFS.open(WM_CONFIG_PORTAL_FILENAME, "r");
      ESP_WML_LOGINFO(F("LoadCPFile "));

      if (!file) {
        ESP_WML_LOGINFO(F("failed"));

        // Trying open redundant config file
        file = FileFS.open(WM_CONFIG_PORTAL_FILENAME_BACKUP, "r");
        ESP_WML_LOGINFO(F("LoadBkUpCPFile "));

        if (!file) {
          ESP_WML_LOGINFO(F("failed"));
          return false;
        }
      }

      file.readBytes((char *) &readForcedConfigPortalFlag, sizeof(readForcedConfigPortalFlag));

      ESP_WML_LOGINFO(F("OK"));
      file.close();

      // Return true if forced CP (0xDEADBEEF read at offset EPROM_START + DRD_FLAG_DATA_SIZE + CONFIG_DATA_SIZE)
      // => set flag noForcedConfigPortal = false
      if (readForcedConfigPortalFlag == FORCED_CONFIG_PORTAL_FLAG_DATA) {
        persForcedConfigPortal = false;
        return true;
      } else if (readForcedConfigPortalFlag == FORCED_PERS_CONFIG_PORTAL_FLAG_DATA) {
        persForcedConfigPortal = true;
        return true;
      }
      return false;
    }

    //////////////////////////////////////////////

    void loadAndSaveDefaultConfigData()
    {
      // Load Default Config Data from Sketch
      memcpy(&ESP_WM_LITE_config, &defaultConfig, sizeof(ESP_WM_LITE_config));
      strcpy(ESP_WM_LITE_config.header, WM_BOARD_TYPE);

      // Including config and dynamic data, and assume valid
      saveConfigData();

      ESP_WML_LOGERROR(F("======= Start Loaded Config Data ======="));
      ESP_WM_LITE_config.print();
    }

    //////////////////////////////////////////////

    // New connectMultiWiFi() logic from v1.7.0
    // Max times to try WiFi per loop() iteration. To avoid blocking issue in loop()
    // Default 1 and minimum 1.
#if !defined(MAX_NUM_WIFI_RECON_TRIES_PER_LOOP)
#define MAX_NUM_WIFI_RECON_TRIES_PER_LOOP     1
#else
#if (MAX_NUM_WIFI_RECON_TRIES_PER_LOOP < 1)
#define MAX_NUM_WIFI_RECON_TRIES_PER_LOOP     1
#endif
#endif

    uint8_t connectMultiWiFi()
    {
      // For ESP32, this better be 0 to shorten the connect time.
      // For ESP32-S2/C3, must be > 500
#if ( USING_ESP32_S2 || USING_ESP32_C3 )
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS           500L
#else
      // For ESP32 core v1.0.6, must be >= 500
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS           800L
#endif

#define WIFI_MULTI_CONNECT_WAITING_MS                   500L

      uint8_t status;

      ESP_WML_LOGINFO(F("Connecting MultiWifi..."));

      WiFi.mode(WIFI_STA);

      setHostname();

      int i = 0;
      status = wifiMulti.run();
      delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);

      uint8_t numWiFiReconTries = 0;

      while ( ( status != WL_CONNECTED ) && (numWiFiReconTries++ < MAX_NUM_WIFI_RECON_TRIES_PER_LOOP) )
      {
        status = WiFi.status();

        if ( status == WL_CONNECTED )
          break;
        else
          delay(WIFI_MULTI_CONNECT_WAITING_MS);
      }

      if ( status == WL_CONNECTED )
      {
        ESP_WML_LOGWARN1(F("WiFi connected after time: "), i);
        ESP_WML_LOGWARN3(F("SSID="), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
        ESP_WML_LOGWARN3(F("Channel="), WiFi.channel(), F(",IP="), WiFi.localIP() );
      }
      else
      {
        ESP_WML_LOGERROR(F("WiFi not connected"));

#if RESET_IF_NO_WIFI

        // To avoid unnecessary DRD
        rd->loop();
        resetFunc();

#endif
      }

      return status;
    }

    //////////////////////////////////////////////

    void fetchUserCode() {
      HTTPClient http;
      http.setConnectTimeout(500);
      http.setTimeout(500);
      // http.begin(FERMI_CLOUD_USERCODE_URL);
      http.begin("http://192.168.1.1/");
      // http.addHeader("Content-Type", "application/x-www-form-urlencoded", false, false);
      // int httpCode = http.POST((uint8_t *)FERMI_CLOUD_USERCODE_PAYLOAD, sizeof(FERMI_CLOUD_USERCODE_PAYLOAD));
      int httpCode = http.GET();
      ESP_WML_LOGINFO1(F("s:DNS IP = "), WiFi.dnsIP(0).toString());
      ESP_WML_LOGINFO1(F("s:Gateway IP = "), WiFi.gatewayIP().toString());
      ESP_WML_LOGINFO1(F("s:Status code = "), httpCode);
      if (httpCode == 200) {
          StaticJsonDocument<200> filter;
          filter["device_code"] = true;
          filter["user_code"] = true;
          StaticJsonDocument<200> doc;
          DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
          if (!error) {
              deviceCode = doc["device_code"].as<String>();
              userCode = doc["user_code"].as<String>();
              ESP_WML_LOGINFO1(F("s:User code = "), userCode.c_str());
          }
      }
      http.end();
    }

    void createConfigJson(String& json)
    {
      json = FPSTR("{\"wifis\":[");
      for (int i = 0, list_items = 0; (i < WiFiNetworksFound) && (list_items < MAX_SSID_IN_LIST); i++)
      {
        if (indices[i] == -1)
          continue;     // skip duplicates and those that are below the required quality

        jsonAppendValue(json, WiFi.SSID(indices[i]));
        list_items++;   // Count number of suitable, distinct SSIDs to be included in list
      }
      json += ']';

    if (hadConfigData) {
      jsonAppendKeyValue(json, FPSTR("id"), ESP_WM_LITE_config.wifiCreds[0].ssid);
      jsonAppendKeyValue(json, FPSTR("pw"), ESP_WM_LITE_config.wifiCreds[0].pw, PASS_OBFUSCATE_STRING);
      jsonAppendKeyValue(json, FPSTR("id1"), ESP_WM_LITE_config.wifiCreds[1].ssid);
      jsonAppendKeyValue(json, FPSTR("pw1"), ESP_WM_LITE_config.wifiCreds[1].pw, PASS_OBFUSCATE_STRING);
#if USING_BOARD_NAME
      jsonAppendKeyValue(json, FPSTR("nm"), ESP_WM_LITE_config.boardName);
#endif
    }

      jsonAppendKeyValue(json, FPSTR("host"), RFC952_hostname);
      json += '}';
    }

    //////////////////////////////////////////////

    void getConfig(AsyncWebServerRequest *request) {
      String json;
      createConfigJson(json);

#if ( ARDUINO_ESP32S2_DEV || ARDUINO_FEATHERS2 || ARDUINO_PROS2 || ARDUINO_MICROS2 )

      request->send(200, FPSTR(WM_HTTP_HEAD_TEXT_HTML), json);

      // Fix ESP32-S2 issue with WebServer (https://github.com/espressif/arduino-esp32/issues/4348)
      delay(1);
#else

      AsyncWebServerResponse *response = request->beginResponse(200, FPSTR(WM_HTTP_HEAD_TEXT_JSON), json);
      response->addHeader(FPSTR(WM_HTTP_CACHE_CONTROL), FPSTR(WM_HTTP_NO_STORE));

#if USING_CORS_FEATURE
      // New from v1.2.0, for configure CORS Header, default to WM_HTTP_CORS_ALLOW_ALL = "*"
      ESP_WML_LOGDEBUG3(F("handleRequest:WM_HTTP_CORS:"), WM_HTTP_CORS, " : ", _CORS_Header);
      response->addHeader(FPSTR(WM_HTTP_CORS), _CORS_Header);
#endif

      response->addHeader(FPSTR(WM_HTTP_PRAGMA), FPSTR(WM_HTTP_NO_CACHE));
      response->addHeader(FPSTR(WM_HTTP_EXPIRES), "-1");
      request->send(response);

#endif    // ARDUINO_ESP32S2_DEV
    }

    void parseParam(AsyncWebServerRequest *request, const char *id, char *pdata, uint8_t maxlen, const char *obfuscateValue = NULL)
    {
      AsyncWebParameter *param = request->getParam(id, true);
      if (param != NULL) {
        if (param->value() == obfuscateValue)
          return;
        ESP_WML_LOGDEBUG3(F("h:"), id, F("="), param->value().c_str());
        strncpy(pdata, param->value().c_str(), maxlen - 1);
        pdata[maxlen - 1] = '\0';
      }
    }

    void updateConfig(AsyncWebServerRequest *request)
    {
      ESP_WM_LITE_config.resetZero();
      strcpy(ESP_WM_LITE_config.header, WM_BOARD_TYPE);

      parseParam(request, FPSTR("id"), ESP_WM_LITE_config.wifiCreds[0].ssid, WM_SSID_MAX_LEN);
      parseParam(request, FPSTR("pw"), ESP_WM_LITE_config.wifiCreds[0].pw, WM_PASSWORD_MAX_LEN, PASS_OBFUSCATE_STRING);
      parseParam(request, FPSTR("id1"), ESP_WM_LITE_config.wifiCreds[1].ssid, WM_SSID_MAX_LEN);
      parseParam(request, FPSTR("pw1"), ESP_WM_LITE_config.wifiCreds[1].pw, WM_PASSWORD_MAX_LEN, PASS_OBFUSCATE_STRING);
#if USING_BOARD_NAME
      parseParam(request, FPSTR("nm"), ESP_WM_LITE_config.boardName, WM_BOARD_NAME_MAX_LEN);
#endif

      {
#if USE_LITTLEFS
        ESP_WML_LOGERROR1(F("h:Updating LittleFS:"), WM_CONFIG_FILENAME);
#elif USE_SPIFFS
        ESP_WML_LOGERROR1(F("h:Updating SPIFFS:"), WM_CONFIG_FILENAME);
#else
        ESP_WML_LOGERROR(F("h:Updating EEPROM. Please wait for reset"));
#endif

        saveAllConfigData();

        // Done with CP, Clear CP Flag here if forced
        if (isForcedConfigPortal)
          clearForcedCP();

        this->begin("", WIFI_AP_STA);
        if (WiFi.status() == WL_CONNECTED) {
          fetchUserCode();
        }

        request->send(204, FPSTR(WM_HTTP_HEAD_TEXT_HTML));

        // TODO: uncomment
        // ESP_WML_LOGERROR(F("h:Rst"));

        // // TO DO : what command to reset
        // // Delay then reset the board after save data
        // resetFunc();
      }
    }

    //////////////////////////////////////////////

#ifndef CONFIG_TIMEOUT
  #warning Default CONFIG_TIMEOUT = 60s
  #define CONFIG_TIMEOUT      60000L
#endif

    void startConfigurationMode()
    {
      configTimeout = 0;  // To allow user input in CP
      WiFiNetworksFound = scanWifiNetworks(&indices);

#if USE_LED_BUILTIN
      // turn the LED_BUILTIN ON to tell us we are in configuration mode.
      digitalWrite(LED_BUILTIN, HIGH);
#endif

      if (portal_ssid.isEmpty())
      {
        String chipID = String(ESP_getChipId(), HEX);
        chipID.toUpperCase();
        portal_ssid = "ESP_" + chipID;
        // if (portal_pass.isEmpty())
        //   portal_pass = "MyESP_" + chipID;
      }

      WiFi.mode(WIFI_AP_STA);
      listSPIFFSFiles();

      // New
      delay(100);

      static int channel;

      // Use random channel if WiFiAPChannel == 0
      if (WiFiAPChannel == 0)
        //channel = random(MAX_WIFI_CHANNEL) + 1;
        channel = (millis() % MAX_WIFI_CHANNEL) + 1;
      else
        channel = WiFiAPChannel;

      // softAPConfig() must be put before softAP() for ESP8266 core v3.0.0+ to work.
      // ESP32 or ESP8266is core v3.0.0- is OK either way
      WiFi.softAPConfig(portal_apIP, portal_apIP, IPAddress(255, 255, 255, 0));

      WiFi.softAP(portal_ssid.c_str(), portal_pass.isEmpty() ? NULL : portal_pass.c_str(), channel);

      ESP_WML_LOGERROR3(F("\nstConf:SSID="), portal_ssid, F(",PW="), portal_pass);
      ESP_WML_LOGERROR3(F("IP="), portal_apIP.toString(), ",ch=", channel);

      delay(100); // ref: https://github.com/espressif/arduino-esp32/issues/985#issuecomment-359157428

      if (!server)
      {
        server = new AsyncWebServer(HTTP_PORT);
      }

      if (!dnsServer)
      {
        dnsServer = new AsyncDNSServer();
      }

      //See https://stackoverflow.com/questions/39803135/c-unresolved-overloaded-function-type?rq=1
      if (server && dnsServer)
      {
        // CaptivePortal
        // if DNSServer is started with "*" for domain name, it will reply with provided IP to all DNS requests
        dnsServer->start(DNS_PORT, "*", portal_apIP);
        
        // reply to all requests with same HTML
        server->onNotFound([this](AsyncWebServerRequest * request)
        {
          String path = request->url();
          Serial.print(F("HANDLE REQUEST: "));
          Serial.println(path);
          AsyncWebServerResponse *response = request->beginResponse(FileFS, path);
          if (response != NULL) {
            request->send(response);
            return;
          }
          response = request->beginResponse(FileFS, "/index.html");
          if (response != NULL) {
            request->send(response);
            return;
          }
        });

        // config handler
        server->on("/config", HTTP_GET, [this](AsyncWebServerRequest * request) {
          Serial.print(F("GET CONFIG"));
          getConfig(request);
        });
        server->on("/config", HTTP_POST, [this](AsyncWebServerRequest * request) {
          Serial.print(F("UPDATE CONFIG"));
          updateConfig(request);
        });
        server->on("/code", HTTP_GET, [this](AsyncWebServerRequest * request) {
          Serial.print(F("FETCH CODE"));
          fetchUserCode();
        });

        server->begin();
      }

      // If there is no saved config Data, stay in config mode forever until having config Data.
      // or SSID, PW, Server,Token ="nothing"
      if (hadConfigData) {
        configTimeout = millis() + CONFIG_TIMEOUT;
        ESP_WML_LOGDEBUG3(F("s:millis() = "), millis(), F(", configTimeout = "), configTimeout);
      } else {
        configTimeout = 0;
        ESP_WML_LOGDEBUG(F("s:configTimeout = 0"));
      }

      configuration_mode = true;
    }

    int           _paramsCount            = 0;
    int           _minimumQuality         = INT_MIN;
    bool          _removeDuplicateAPs     = true;

    //Scan for WiFiNetworks in range and sort by signal strength
    //space for indices array allocated on the heap and should be freed when no longer required
    int scanWifiNetworks(int **indicesptr)
    {
      ESP_WML_LOGDEBUG(F("Scanning WiFis..."));
      int n = WiFi.scanNetworks();
      ESP_WML_LOGDEBUG1(F("scanWifiNetworks: Done, Scanned Networks n = "), n);

      if (n <= 0) {
        ESP_WML_LOGDEBUG(F("No network found"));
        return 0;
      }

      // Allocate space off the heap for indices array.
      // This space should be freed when no longer required.
      int* indices = (int *)malloc(n * sizeof(int));
      if (indices == NULL) {
        ESP_WML_LOGDEBUG(F("ERROR: Out of memory"));
        *indicesptr = NULL;
        return 0;
      }

      *indicesptr = indices;

      //sort networks
      for (int i = 0; i < n; i++)
        indices[i] = i;

      ESP_WML_LOGDEBUG(F("Sorting"));

      // RSSI SORT
      // old sort
      for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i]))
            std::swap(indices[i], indices[j]);

      ESP_WML_LOGDEBUG(F("Removing Dup"));

      // remove duplicates ( must be RSSI sorted )
      if (_removeDuplicateAPs) {
        String cssid;

        for (int i = 0; i < n; i++) {
          if (indices[i] == -1) continue;
          cssid = WiFi.SSID(indices[i]);

          for (int j = i + 1; j < n; j++)
            if (cssid == WiFi.SSID(indices[j])) {
              ESP_WML_LOGDEBUG1("DUP AP:", WiFi.SSID(indices[j]));
              indices[j] = -1; // set dup aps to index -1
            }
        }
      }

      for (int i = 0; i < n; i++) {
        if (indices[i] == -1) continue; // skip dups
        int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));
        if (_minimumQuality > quality) {
          indices[i] = -1;
          ESP_WML_LOGDEBUG(F("Skipping low quality"));
        }
      }

      ESP_WML_LOGWARN(F("WiFi networks found:"));

      for (int i = 0; i < n; i++) {
        if (indices[i] == -1) continue; // skip dups
        ESP_WML_LOGWARN5(i + 1, ": ", WiFi.SSID(indices[i]), ", ", WiFi.RSSI(i), "dB");
      }

      return n;
    }

};

#endif    //ESPAsync_WiFiManager_Lite_h
