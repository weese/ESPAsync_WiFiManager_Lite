/****************************************************************************************************************************
  WifiManager.h
  For ESP32 boards

  Based on the ESPAsync_WiFiManager_Lite library by Khoi Hoang https://github.com/khoih-prog/ESPAsync_WiFiManager_Lite

 *****************************************************************************************************************************/

#pragma once

#ifndef wm_h_
#define wm_h_

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

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

///////////////////////////////////////////

#define WM_HTTP_PORT     80
#define WM_DNS_PORT      53

///////////////////////////////////////////

#include <ESPAsyncDNSServer.h>
#include <memory>
#undef min
#undef max
#include <algorithm>

#include <esp_wifi.h>
#include "wm_debug.h"
#include "wm_helpers.h"
#include "wm_config.h"
#include "wm_wifi.h"
#include "wm_fermion.h"

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
const char FERMI_CLOUD_TOKEN_URL[]        PROGMEM = "http://fermicloud.spdns.de/auth/realms/fermi-cloud/protocol/openid-connect/token";
const char FERMI_CLOUD_TOKEN_PAYLOAD[]    PROGMEM = "client_id=fermi-device&scope=openid&grant_type=urn:ietf:params:oauth:grant-type:device_code&device_code=";

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

enum WMState {
  WM_READY = 0,
  WM_WIFI_CONFIG,
  WM_CONNECTING,
  WM_FETCH_CODE,
  WM_FETCH_TOKEN,
};

//////////////////////////////////////////

class WiFiManager
{
  public:

    WiFiManager() {}

    ~WiFiManager() {
      delete dnsServer;
      delete server;
      delete events;
      free(indices);
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

    String accessToken;

    bool cloudConnect() {
      // 1. Try to load Wifi config
      if (!config.load()) return false;

      // 2. Try to load access token
      if (!fileExist(WM_TOKEN_FILENAME) &&
          !fileExist(WM_TOKEN_FILENAME_BACKUP)) return false;
      
      // 3. Connect Wifi
      if (!wmConnectWifi(wifiMulti, config)) return false;
      
      // 3. Connect to MQTT server
      if (!Particle.connect()) return false;
    }

    void begin(wifi_mode_t mode = WIFI_STA)
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

      wmSetHostname();
      hadConfigData = loadConfigData();

      isForcedConfigPortal = isForcedCP();

      //  noConfigPortal when loadConfigData() OK and no MRD/DRD'ed
      if (hadConfigData && noConfigPortal && (!isForcedConfigPortal) )
      {
        hadConfigData = true;

        ESP_WML_LOGDEBUG(noConfigPortal ? F("bg: noConfigPortal = true") : F("bg: noConfigPortal = false"));

        if (!wmConnectWifi(wifiMulti, config, mode))
          startConfigurationMode(); // failed to connect to WiFi, will start configuration mode
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

      // Lost connection in running. Give chance to reconfig.
      // Check WiFi status every 5s and update status
      // Check twice to be sure wifi disconnected is real
#define WIFI_STATUS_CHECK_INTERVAL    5000L

      uint32_t curMillis = millis();

      // Call the double reset detector loop method every so often,
      // so that it can recognise when the timeout expires.
      // You can also call rd.stop() when you wish to no longer
      // consider the next reset as a double reset.
      rd->loop();
      loopState();
      return;

      // Lost connection in running. Give chance to reconfig.
      if ( WiFi.status() != WL_CONNECTED )
      {
        // If configTimeout but user hasn't connected to configWeb => try to reconnect WiFi
        // But if user has connected to configWeb, stay there until done, then reset hardware
        if ( state != WM_READY && ( configTimeout == 0 ||  millis() < configTimeout ) )
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
//         if ( state != WM_READY && (configTimeout != 0) ) {
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

            if (wmConnectWifiMulti(wifiMulti) == WL_CONNECTED)
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

          if (wmConnectWifiMulti(wifiMulti) == WL_CONNECTED) {
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
      // else
//       if (state != WM_READY)
//       {
//         // WiFi is connected and we are in configuration_mode
//         state = WM_READY;
//         ESP_WML_LOGINFO(F("run: got WiFi back"));

// #if USE_LED_BUILTIN
//         // turn the LED_BUILTIN OFF to tell us we exit configuration mode.
//         digitalWrite(LED_BUILTIN, LOW);
// #endif

//         if (dnsServer) {
//           dnsServer->stop();
//           delete dnsServer;
//           dnsServer = nullptr;
//         }

//         if (server) {
//           server->end();
//           delete server;
//           server = nullptr;
//         }
//       }
    }

    //////////////////////////////////////////////

    bool isConfigDataValid()  { return hadConfigData; }
    bool isConfigMode()       { return state != WM_READY; }
    String localIP()          { return WiFi.localIP().toString(); }

    void setMinimumSignalQuality(const int& quality) { _minimumQuality = quality; }

    void setConfigPortalIP(const IPAddress& portalIP = IPAddress(192, 168, 4, 1)) { portal_apIP = portalIP; }
    bool loadConfigData()     { return config.load(); }
    void clearConfigData()    { config.clear(); }
    void saveConfigData()     { config.save(); }
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

    void assertConfig() { if (!hadConfigData) loadConfigData(); }

    String getBoardName() {
      assertConfig();
      return config.boardName;
    }

    //////////////////////////////////////////////

    WMConfig & getConfig() {
      assertConfig();
      return config;
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
    WiFiMulti wifiMulti;
    AsyncWebServer *server = nullptr;
    AsyncEventSource *events = nullptr;
    AsyncDNSServer *dnsServer = nullptr;

    unsigned long configTimeout;
    bool hadConfigData = false;
    bool hadDynamicData = false;

    bool isForcedConfigPortal   = false;
    bool persForcedConfigPortal = false;

    WMConfig config;

    IPAddress portal_apIP = IPAddress(192, 168, 4, 1);
    int WiFiAPChannel = 10;

    String portal_ssid = "";
    String portal_pass = "";

    WMState state = WM_READY;
    unsigned long checkStateTimeout = 0;
    String deviceCode = "";
    String userCode = "";

#if USING_CORS_FEATURE
    PGM_P _CORS_Header = WM_HTTP_CORS_ALLOW_ALL;   // "*";
#endif

    int WiFiNetworksFound = 0;    // Number of SSIDs found by WiFi scan, including low quality and duplicates
    int *indices;                 // WiFi network data, filled by scan (SSID, BSSID)

    //////////////////////////////////////

    bool isWiFiConfigValid()
    {
      if (!config.wifiConfigValid()) {
        // If SSID, PW ="blank" or NULL, set the flag
        ESP_WML_LOGERROR(F("Invalid Stored WiFi Config Data"));

        // Nullify the invalid data to avoid displaying garbage
        config.resetZero();

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
      memcpy(&config, &defaultConfig, sizeof(config));
      strcpy(config.header, WM_BOARD_TYPE);

      // Including config and dynamic data, and assume valid
      saveConfigData();

      ESP_WML_LOGERROR(F("======= Start Loaded Config Data ======="));
      config.print();
    }

    //////////////////////////////////////////////

    bool fetchUserCode() {
      HTTPClient http;
      bool result = false;

      http.setConnectTimeout(500);
      http.setTimeout(500);
      http.begin(FERMI_CLOUD_USERCODE_URL);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded", false, false);

      int httpCode = http.POST(FERMI_CLOUD_USERCODE_PAYLOAD);
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
              deviceCode = doc["device_code"].as<const char*>();
              userCode = doc["user_code"].as<const char*>();
              ESP_WML_LOGINFO1(F("s:User code = "), userCode.c_str());
          }
      }
      http.end();
      return result;
    }

    void saveAs(String const &data, char const *filename)
    {
        ESP_WML_LOGINFO1(F("Save file %s... "), filename);
        File file = FileFS.open(filename, "w");
        if (file)
        {
            file.write((uint8_t *)data.c_str(), data.length());
            file.close();
            ESP_WML_LOGINFO(F("OK"));
        }
        else
            ESP_WML_LOGINFO(F("failed"));
    }

    bool hasToken() {
        return FileFS.exists(WM_TOKEN_FILENAME);
    }

    void saveToken(String const &token)
    {
        saveAs(token, WM_TOKEN_FILENAME);
        saveAs(token, WM_TOKEN_FILENAME_BACKUP);
    }
    
    bool fetchAccessToken() {
      HTTPClient http;
      bool result = false;
      http.setConnectTimeout(500);
      http.setTimeout(500);
      http.begin(FERMI_CLOUD_TOKEN_URL);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded", false, false);
      // int httpCode = http.POST((uint8_t *)FERMI_CLOUD_USERCODE_PAYLOAD, sizeof(FERMI_CLOUD_USERCODE_PAYLOAD) - 1);
      int httpCode = http.POST(String(FERMI_CLOUD_TOKEN_PAYLOAD) + deviceCode);
      ESP_WML_LOGINFO1(F("s:Status code = "), httpCode);
      StaticJsonDocument<200> filter;
      StaticJsonDocument<1000> doc;
      if (httpCode == 200) {
          filter["refresh_token"] = true;
          DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
          if (!error) {
              String refreshToken = doc["refresh_token"].as<const char*>();
              ESP_WML_LOGINFO1(F("s:Refresh token = "), refreshToken.c_str());
              saveToken(refreshToken);
              result = true;
          }
      } else if (httpCode == 400) {
          filter["error"] = true;
          DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
          if (!error) {
              if (strcmp(doc["error"].as<const char*>(), "expired_token") == 0) {
                ESP_WML_LOGINFO(F("s:Token expired, request new one"));
                fetchUserCode();
              }
          }
      }
      http.end();
      return result;
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
      jsonAppendKeyValue(json, FPSTR("id"), config.getSSID(0));
      jsonAppendKeyValue(json, FPSTR("pw"), config.getPW(0), PASS_OBFUSCATE_STRING);
      jsonAppendKeyValue(json, FPSTR("id1"), config.getSSID(1));
      jsonAppendKeyValue(json, FPSTR("pw1"), config.getPW(1), PASS_OBFUSCATE_STRING);
#if USING_BOARD_NAME
      jsonAppendKeyValue(json, FPSTR("nm"), config.boardName);
#endif
    }

      json += '}';
    }

    //////////////////////////////////////////////

    void handlerConfigGet(AsyncWebServerRequest *request) {
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

    void parseParam(AsyncWebServerRequest *request, const __FlashStringHelper *id, char *pdata, uint8_t maxlen, const char *obfuscateValue = NULL)
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

    void handlerConfigSet(AsyncWebServerRequest *request)
    {
      config.resetZero();
      strcpy(config.header, WM_BOARD_TYPE);

      parseParam(request, FPSTR("id"), config.wifiCreds[0].ssid, WM_SSID_MAX_LEN);
      parseParam(request, FPSTR("pw"), config.wifiCreds[0].pw, WM_PASSWORD_MAX_LEN, PASS_OBFUSCATE_STRING);
      parseParam(request, FPSTR("id1"), config.wifiCreds[1].ssid, WM_SSID_MAX_LEN);
      parseParam(request, FPSTR("pw1"), config.wifiCreds[1].pw, WM_PASSWORD_MAX_LEN, PASS_OBFUSCATE_STRING);
#if USING_BOARD_NAME
      parseParam(request, FPSTR("nm"), config.boardName, WM_BOARD_NAME_MAX_LEN);
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

    void setState(WMState newState) {
      if (this->state == newState)
        return;
      uint32_t curMillis = millis();
      events->send(String(newState).c_str(), "s", curMillis, 1000);
      switch (newState) {
        case WM_READY:
        case WM_WIFI_CONFIG:
        case WM_FETCH_CODE:
          break;
        case WM_CONNECTING:
          this->begin(WIFI_AP_STA);
          break;
        case WM_FETCH_TOKEN:
          events->send(this->userCode.c_str(), "c", curMillis, 1000);
          break;
      }
      this->state = newState;
      checkStateTimeout = 0;
    }

    void loopState() {
      uint32_t curMillis = millis();
      if (curMillis - checkStateTimeout < WIFI_STATUS_CHECK_INTERVAL)
        return;
      checkStateTimeout = curMillis;

      switch (this->state) {
        case WM_READY:
        case WM_WIFI_CONFIG:
          break;
        case WM_CONNECTING:
          if (WiFi.status() == WL_CONNECTED)
            setState(hasToken() ? WM_READY : WM_FETCH_CODE);
          break;
        case WM_FETCH_CODE:
          if (fetchUserCode())
            setState(WM_FETCH_TOKEN);
          break;
        case WM_FETCH_TOKEN:
          if (fetchAccessToken())
            setState(WM_READY);
      }
    }

    void startConfigurationMode()
    {
      configTimeout = 0;  // To allow user input in CP
      WiFiNetworksFound = wmScanWifiNetworks(&indices);

#if USE_LED_BUILTIN
      // turn the LED_BUILTIN ON to tell us we are in configuration mode.
      digitalWrite(LED_BUILTIN, HIGH);
#endif

      if (portal_ssid.isEmpty())
        portal_ssid = WM_HOSTNAME();

      WiFi.mode(WIFI_AP_STA);
      listSPIFFSFiles();

      // New
      delay(100);

      int channel = WiFiAPChannel;
      // Use random channel if WiFiAPChannel == 0
      if (channel == 0)
        channel = (millis() % MAX_WIFI_CHANNEL) + 1;

      // softAPConfig() must be put before softAP() for ESP8266 core v3.0.0+ to work.
      // ESP32 or ESP8266is core v3.0.0- is OK either way
      WiFi.softAPConfig(portal_apIP, portal_apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP(portal_ssid.c_str(), portal_pass.isEmpty() ? NULL : portal_pass.c_str(), channel);

      ESP_WML_LOGERROR3(F("\nstConf:SSID="), portal_ssid, F(",PW="), portal_pass);
      ESP_WML_LOGERROR3(F("IP="), portal_apIP.toString(), ",ch=", channel);

      delay(100); // ref: https://github.com/espressif/arduino-esp32/issues/985#issuecomment-359157428

      if (!server)
      {
        server = new AsyncWebServer(WM_HTTP_PORT);
        events = new AsyncEventSource(PSTR("/status"));
      }

      if (!dnsServer)
        dnsServer = new AsyncDNSServer();

      //See https://stackoverflow.com/questions/39803135/c-unresolved-overloaded-function-type?rq=1
      if (server && dnsServer)
      {
        // CaptivePortal
        // if DNSServer is started with "*" for domain name, it will reply with provided IP to all DNS requests
        dnsServer->start(WM_DNS_PORT, "*", portal_apIP);
        
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
          handlerConfigGet(request);
        });
        server->on("/config", HTTP_POST, [this](AsyncWebServerRequest * request) {
          Serial.print(F("UPDATE CONFIG"));
          handlerConfigSet(request);
          setState(WM_CONNECTING);
        });
        server->on("/code", HTTP_GET, [this](AsyncWebServerRequest * request) {
          Serial.print(F("FETCH CODE"));
          fetchUserCode();
        });

        // Handle Web Server Events
        events->onConnect([this](AsyncEventSourceClient *client){
          client->send(String(this->state).c_str(), "s", millis(), 1000);
          client->send(this->userCode.c_str(), "c", millis(), 1000);
        });
        server->addHandler(events);
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

      state = WM_WIFI_CONFIG;
    }

};

#endif //wm_h_
