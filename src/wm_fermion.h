#pragma once

#ifndef wm_fermion_h_
#define wm_fermion_h_

// #include <Ethernet.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include "mqtt_client.h"
#include "wm_file.h"
#include "wm_wifi.h"
#include "wm_flags.h"

#define WM_REFRESH_TOKEN_FILENAME "/wm_token.dat"
#define WM_REFRESH_TOKEN_FILENAME_BACKUP "/wm_token.bak"

const char FERMI_CLOUD_TOKEN_URL[]            PROGMEM = "https://fermicloud.dev/auth/realms/fermi-cloud/protocol/openid-connect/token";
const char FERMI_CLOUD_DEVICE_CODE_PAYLOAD[]  PROGMEM = "client_id=fermi-device&scope=openid&grant_type=urn:ietf:params:oauth:grant-type:device_code&device_code=";
const char FERMI_CLOUD_REFRESH_PAYLOAD[]      PROGMEM = "client_id=fermi-device&grant_type=refresh_token&refresh_token=";
#define FERMI_VERSION "1.0.0"
#define FERMI_DEVICE "ESP32"

//////////////////////////////////////////////

struct System_t {
    static String deviceID(void) { return "foo"; }
};

static System_t System;

//////////////////////////////////////////////

#define FERMI_CLOUD_FUNCTION_HANDLERS 10
#define FERMI_CLOUD_MQTT_URI "mqtts://fermicloud.dev:8883"

enum FetchAccessTokenResult {
    FC_OK = 0,
    FC_CODE_NOT_VERIFIED_YET,
    FC_CODE_EXPIRED,
    FC_INVALID_REFRESH_TOKEN,
    FC_CANNOT_LOAD_REFRESH_TOKEN,
    FC_INVALID_RESPONSE
};

// This is isrgrootx1.pem, the root Certificate Authority that signed 
// the certifcate of the FermiCloud server https://fermicloud.dev
// This certificate is valid until 04/06/2035, 18:04:38 GMT+7
const char* fermiRootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"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\n" \
"-----END CERTIFICATE-----\n";

typedef enum
{
    MY_DEVICES,
    ALL_DEVICES
} SubscribeScopeEnum;

struct PublishFlagType; // Tag type for Particle.publish() flags
typedef particle::Flags<PublishFlagType, uint8_t> PublishFlags;
typedef PublishFlags::FlagType PublishFlag;

const uint32_t PUBLISH_EVENT_FLAG_PUBLIC = 0x0;
const uint32_t PUBLISH_EVENT_FLAG_PRIVATE = 0x1;
const uint32_t PUBLISH_EVENT_FLAG_NO_ACK = 0x2;
const uint32_t PUBLISH_EVENT_FLAG_WITH_ACK = 0x8;
const uint32_t PUBLISH_EVENT_FLAG_RETAIN = 0xa;

const PublishFlag PUBLIC(PUBLISH_EVENT_FLAG_PUBLIC);
const PublishFlag PRIVATE(PUBLISH_EVENT_FLAG_PRIVATE);
const PublishFlag NO_ACK(PUBLISH_EVENT_FLAG_NO_ACK);
const PublishFlag WITH_ACK(PUBLISH_EVENT_FLAG_WITH_ACK);
const PublishFlag WITH_RETAIN(PUBLISH_EVENT_FLAG_RETAIN);

typedef void (*EventHandler)(const char *event_name, const char *data);
typedef void (*EventHandlerWithData)(void *handler_data, const char *event_name, const char *data);
typedef int (*user_function_int_str_t)(String);

struct FunctionHandler
{
    union
    {
        EventHandler func;
        EventHandlerWithData funcWithData;
    };
    String topic;
    void *data;
};

struct CloudFunctionHandler
{
    user_function_int_str_t func;
    String funcKey;
};

// Structure for function request/response
struct FunctionRequest {
    uint32_t requestId;
    String parameters;
};

struct FunctionResponse {
    uint32_t requestId;
    int result;
};

struct Fermion
{
    FunctionHandler handlers[FERMI_CLOUD_FUNCTION_HANDLERS];
    CloudFunctionHandler functionHandlers[FERMI_CLOUD_FUNCTION_HANDLERS];
    uint8_t handlerCount;
    uint8_t functionHandlerCount;
    bool _gotDisconnected;
    bool _isConnected;
    esp_mqtt_client_handle_t mqttClient;
    String deviceID;
    String accessToken;


    Fermion() : 
        handlerCount(0),
        functionHandlerCount(0),
        _gotDisconnected(false),
        _isConnected(false),
        deviceID(wmHostname()),
        mqttClient(NULL)
    {
        // esp_log_level_set("*", ESP_LOG_INFO);
        // esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
        // esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
        // esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
        // esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
        // esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
        // esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    }
    
    ~Fermion() {
        if (isConnected())
            disconnect();
    }

    bool hasRefreshToken() {
        return FileFS.exists(WM_REFRESH_TOKEN_FILENAME) || FileFS.exists(WM_REFRESH_TOKEN_FILENAME_BACKUP);
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

    void saveRefreshToken(String const &token)
    {
        saveAs(token, WM_REFRESH_TOKEN_FILENAME);
        saveAs(token, WM_REFRESH_TOKEN_FILENAME_BACKUP);
    }

    void deleteRefreshToken() {
        FileFS.remove(WM_REFRESH_TOKEN_FILENAME);
        FileFS.remove(WM_REFRESH_TOKEN_FILENAME_BACKUP);
    }
    
    FetchAccessTokenResult fetchAccessToken(const char* deviceCode = NULL, long timeout = 5000) {
      WiFiClientSecure client;
      client.setCACert(fermiRootCACertificate);
      {
        // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure client is 
        HTTPClient https;
        FetchAccessTokenResult result = FC_INVALID_RESPONSE;
        https.setConnectTimeout(timeout);
        https.setTimeout(timeout);
        https.begin(client, FERMI_CLOUD_TOKEN_URL);
        https.addHeader("Content-Type", "application/x-www-form-urlencoded", false, false);
 
        int httpCode;
        if (deviceCode && deviceCode[0] != '\0') {
            ESP_WML_LOGINFO1(F("s:device code = "), String(FERMI_CLOUD_DEVICE_CODE_PAYLOAD) + deviceCode);

            httpCode = https.POST(String(FERMI_CLOUD_DEVICE_CODE_PAYLOAD) + deviceCode);
        } else {
            String refreshToken;
            if (!loadFile(refreshToken, WM_REFRESH_TOKEN_FILENAME) &&
                !loadFile(refreshToken, WM_REFRESH_TOKEN_FILENAME_BACKUP)) 
            {
                ESP_WML_LOGERROR(F("s:Cannot load refresh token"));
                return FC_CANNOT_LOAD_REFRESH_TOKEN;
            }
            httpCode = https.POST(String(FERMI_CLOUD_REFRESH_PAYLOAD) + refreshToken);
        }

        ESP_WML_LOGINFO1(F("s:Status code = "), httpCode);
        StaticJsonDocument<200> filter;
        DynamicJsonDocument doc(4000);
        if (httpCode == 200) {
            filter["access_token"] = true;
            filter["refresh_token"] = true;
            DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(filter));
            if (!error) {
                accessToken = doc["access_token"].as<const char*>();
                String refreshToken = doc["refresh_token"].as<const char*>();
                ESP_WML_LOGINFO1(F("s:Refresh token = "), refreshToken.c_str());
                saveRefreshToken(refreshToken);
                result = FC_OK;
            } else {
                ESP_WML_LOGINFO1(F("s:Deserialisation error: "), error.c_str());
            }
        } else if (httpCode == 400) {
            filter["error"] = true;
            filter["error_description"] = true;
            DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(filter));
            if (!error) {
                const char *err = doc["error"].as<const char*>();
                if (strcmp(err, "expired_token") == 0) {
                  ESP_WML_LOGINFO(F("s:User code expired"));
                  result = FC_CODE_EXPIRED;
                } else if (strcmp(err, "authorization_pending") == 0)
                  result = FC_CODE_NOT_VERIFIED_YET;
                else if (strcmp(err, "invalid_grant") == 0) {
                  result = FC_INVALID_REFRESH_TOKEN;
                } else {
                  ESP_WML_LOGINFO1(F("s:Got error: "), err);
                  ESP_WML_LOGINFO1(F("s:Got error description: "), doc["error_description"].as<const char*>());
                }
            } else {
                ESP_WML_LOGINFO1(F("s:Deserialisation error: "), error.c_str());
            }
        }
        https.end();
        return result;
      }
    }

    bool isConnected() {
        if (_gotDisconnected) {
            esp_mqtt_client_destroy(mqttClient);
            mqttClient = NULL;
            _gotDisconnected = false;
            _isConnected = false;
        }
        return mqttClient != NULL && _isConnected;
    }

    bool connect()
    {
        if (!isConnected()) {
            // ESP_WML_LOGINFO1(F("s:Access token: %s"), accessToken.c_str());
            // ESP_WML_LOGINFO1(F("s:Heap free: %s"), ESP.getFreeHeap());
            esp_mqtt_client_config_t mqtt_cfg = {
                .uri = FERMI_CLOUD_MQTT_URI,
                // .client_id = deviceID.c_str(),
                // .username = "",
                .password = accessToken.c_str(),
                .cert_pem = fermiRootCACertificate,
                .protocol_ver = MQTT_PROTOCOL_V_3_1,
                .out_buffer_size = 2048,
            };
            mqttClient = esp_mqtt_client_init(&mqtt_cfg);
            if (!mqttClient) {
                ESP_WML_LOGINFO(F("s:esp_mqtt_client_init() failed"));
                return false;
            }
            ESP_WML_LOGINFO(F("s:esp_mqtt_client_init() ok."));

            esp_err_t err = esp_mqtt_client_register_event(mqttClient, MQTT_EVENT_ANY, mqttHandlerStatic, this);
            if (err != ESP_OK) {
                esp_mqtt_client_destroy(mqttClient);
                mqttClient = NULL;
                ESP_WML_LOGINFO1(F("s:esp_mqtt_client_register_event() failed: "), err);
                return false;
            }

            err = esp_mqtt_client_start(mqttClient);
            if (err != ESP_OK) {
                esp_mqtt_client_destroy(mqttClient);
                mqttClient = NULL;
                ESP_WML_LOGINFO1(F("s:esp_mqtt_client_start() failed: "), err);
                return false;
            }
            ESP_WML_LOGINFO(F("s:esp_mqtt_client_start() ok."));

            return true;
        }
        return false;
    }

    void hello() {
        if (isConnected()) {
            int result = publish("fermion_hello", 
                "{\"version\":\"" FERMI_VERSION "\"" \
                ",\"device\":\"" FERMI_DEVICE "\"}", PRIVATE | WITH_RETAIN);
        }
    }

    void heartBeat() {
        if (isConnected()) {
            int8_t rssi = WiFi.RSSI();
            auto freeRam = ESP.getFreeHeap();
            char data[100];
            snprintf(data, sizeof(data), "{\"rssi\":%i,\"free_ram\":%i}", rssi, freeRam);
            ESP_WML_LOGINFO1(F("s:heartBeat() = "), data);
            int result = publish("fermion_heartbeat", data, PRIVATE);
            ESP_WML_LOGINFO1(F("s:result = "), result);
        }
    }

    void disconnect() {
        if (isConnected()) {
            esp_mqtt_client_stop(mqttClient);
            esp_mqtt_client_disconnect(mqttClient);
            esp_mqtt_client_destroy(mqttClient);
        }
        mqttClient = NULL;
    }

    int publish(const char *eventName, const char *eventData, PublishFlags flags)
    {
        char topic[300];
        _getEventTopic(topic, sizeof(topic), eventName);
        return esp_mqtt_client_publish(mqttClient, topic, eventData, 0, 0, flags & WITH_RETAIN ? 1 : 0);
    }

    bool subscribe(const char *eventName, EventHandler handler, SubscribeScopeEnum scope)
    {
        char topic[300];
        _getEventTopic(topic, sizeof(topic), eventName);
        if (esp_mqtt_client_subscribe(mqttClient, topic, 0) < 0)
            return false;
        handlers[handlerCount] = FunctionHandler{
            .func = handler,
            .topic = topic,
            .data = NULL,
        };
        handlerCount++;
        return true;
    }

    bool unsubscribe(const char *eventName)
    {
        char topic[300];
        _getEventTopic(topic, sizeof(topic), eventName);
        return esp_mqtt_client_unsubscribe(mqttClient, topic) >= 0;
    }

    // Function registration methods
    bool function(const char *funcKey, user_function_int_str_t func)
    {
        // Check if function with same name already exists
        for (uint8_t i = 0; i < functionHandlerCount; i++) {
            if (functionHandlers[i].funcKey == funcKey) {
                // Replace existing function
                functionHandlers[i].func = func;
                ESP_WML_LOGINFO1(F("s:Function replaced: "), funcKey);
                return true;
            }
        }
        
        // Add new function if we have space
        if (functionHandlerCount >= FERMI_CLOUD_FUNCTION_HANDLERS) {
            ESP_WML_LOGERROR(F("s:Too many function handlers registered"));
            return false;
        }
        
        functionHandlers[functionHandlerCount] = CloudFunctionHandler{
            .func = func,
            .funcKey = String(funcKey)
        };
        functionHandlerCount++;
        
        // Subscribe to function topic
        char topic[300];
        _getFunctionTopic(topic, sizeof(topic), funcKey);
        if (isConnected()) {
            esp_mqtt_client_subscribe(mqttClient, topic, 0);
        }
        
        ESP_WML_LOGINFO1(F("s:Function registered: "), funcKey);
        return true;
    }

    template<typename T>
    bool function(const T &name, user_function_int_str_t func)
    {
        return function(name, func);
    }

    // Remove/unregister a function
    bool removeFunction(const char *funcKey)
    {
        for (uint8_t i = 0; i < functionHandlerCount; i++) {
            if (functionHandlers[i].funcKey == funcKey) {
                // Unsubscribe from function topic
                char topic[300];
                _getFunctionTopic(topic, sizeof(topic), funcKey);
                if (isConnected()) {
                    esp_mqtt_client_unsubscribe(mqttClient, topic);
                }
                
                // Remove function by shifting remaining functions
                for (uint8_t j = i; j < functionHandlerCount - 1; j++) {
                    functionHandlers[j] = functionHandlers[j + 1];
                }
                functionHandlerCount--;
                
                ESP_WML_LOGINFO1(F("s:Function removed: "), funcKey);
                return true;
            }
        }
        
        ESP_WML_LOGINFO1(F("s:Function not found for removal: "), funcKey);
        return false;
    }

    // Check if a function is registered
    bool hasFunction(const char *funcKey)
    {
        for (uint8_t i = 0; i < functionHandlerCount; i++) {
            if (functionHandlers[i].funcKey == funcKey) {
                return true;
            }
        }
        return false;
    }

    // bool subscribe(System.deviceID() + "/" PREFIX_WIDGET, handlerWidget, MY_DEVICES);

    // Particle.function("notify", handlerNotificationFunction);

    void handlerCallback(char *topic, const char *payload, unsigned int length)
    {
        for (uint8_t i = 0; i < handlerCount; i++)
        {
            FunctionHandler &handler = handlers[i];
            if (handler.topic == topic)
            {
                if (handler.data)
                    handler.funcWithData(handler.data, topic, payload);
                else
                    handler.func(topic, payload);
            }
        }
    }

    bool functionCallback(char *topic, const char *payload, unsigned int length)
    {
        for (uint8_t i = 0; i < functionHandlerCount; i++)
        {
            CloudFunctionHandler &handler = functionHandlers[i];
            
            if (_compareFunctionTopic(topic, handler.funcKey.c_str()))
            {
                // Parse the request payload
                String payloadStr(payload, length);
                FunctionRequest request;
                FunctionResponse response;
                
                // Try to parse as JSON first
                StaticJsonDocument<512> doc;
                DeserializationError error = deserializeJson(doc, payloadStr);
                
                if (!error) {
                    // JSON format: {"i": 123, "p": "param_value"}
                    request.requestId = doc["i"] | 0;
                    request.parameters = doc["p"] | payloadStr; // fallback to raw payload
                } else {
                    // Simple string format (backward compatibility)
                    request.requestId = millis(); // generate simple ID
                    request.parameters = payloadStr;
                }
                
                // Execute the function
                int result = handler.func(request.parameters);
                
                // Prepare response
                response.requestId = request.requestId;
                response.result = result;
                
                // Send function result back
                char responseTopic[300];
                snprintf(responseTopic, sizeof(responseTopic), "users/mqtt_user/devices/%s/functions/%s/result", deviceID.c_str(), handler.funcKey.c_str());
                
                // Create JSON response
                StaticJsonDocument<256> responseDoc;
                responseDoc["i"] = response.requestId;
                responseDoc["r"] = response.result;
                
                String responseJson;
                serializeJson(responseDoc, responseJson);
                
                esp_mqtt_client_publish(mqttClient, responseTopic, responseJson.c_str(), 0, 0, 0);
                
                ESP_WML_LOGINFO1(F("s:Function called: "), handler.funcKey.c_str());
                ESP_WML_LOGINFO1(F("s:Request ID: "), request.requestId);
                ESP_WML_LOGINFO1(F("s:Function result: "), result);
                return true;
            }
        }
        return false;
    }

    static void mqttHandlerStatic(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
        ((Fermion *)handler_args)->mqttHandler(base, event_id, event_data);
    }

    esp_err_t mqttHandler(esp_event_base_t base, int32_t event_id, void *event_data)
    {
        Serial.printf("Event dispatched from event loop base=%s, event_id=%d", base, event_id);
        esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
        esp_mqtt_client_handle_t client = event->client;
        int msg_id;

        switch (event_id)
        {
        case MQTT_EVENT_CONNECTED:
            Serial.println("MQTT_EVENT_CONNECTED");
            _isConnected = true;
            // Subscribe to all handler topics
            for (uint8_t i = 0; i < handlerCount; i++)
                esp_mqtt_client_subscribe(client, handlers[i].topic.c_str(), 0);
            // Subscribe to all function topics
            for (uint8_t i = 0; i < functionHandlerCount; i++) {
                char topic[300];
                _getFunctionTopic(topic, sizeof(topic), functionHandlers[i].funcKey.c_str());
                esp_mqtt_client_subscribe(client, topic, 0);
            }
            hello();
            
            break;

        case MQTT_EVENT_DISCONNECTED:
            Serial.println("MQTT_EVENT_DISCONNECTED");
            _isConnected = false;
            _gotDisconnected = true;
            // fermion->accessToken.clear();
            break;

        case MQTT_EVENT_SUBSCRIBED:
            Serial.println("MQTT_EVENT_SUBSCRIBED");
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            Serial.println("MQTT_EVENT_UNSUBSCRIBED");
            break;

        case MQTT_EVENT_PUBLISHED:
            Serial.println("MQTT_EVENT_PUBLISHED");
            break;

        case MQTT_EVENT_DATA:
            Serial.println("MQTT_EVENT_DATA");
            Serial.printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            Serial.printf("DATA=%.*s\r\n", event->data_len, event->data);
            
            // Check if this is a function call
            if (functionCallback(event->topic, event->data, event->data_len)) {
                // Function was handled
            } else {
                // Handle as regular event
                handlerCallback(event->topic, event->data, event->data_len);
            }
            break;

        case MQTT_EVENT_ERROR:
            Serial.println("MQTT_EVENT_ERROR");
            break;
        }
        return ESP_OK;
    }

private:

    void _getDeviceTopic(char *buffer, size_t length, const char *eventName) {
        strncpy(buffer, "users/mqtt_user/devices/", length - 1);
        strncat(buffer, deviceID.c_str(), length - 1);
        strncat(buffer, "/", length - 1);
        strncat(buffer, eventName, length - 1);
        buffer[length - 1] = '\0'; // Ensure null-terminated
    }

    void _getEventTopic(char *buffer, size_t length, const char *eventName) {
        strncpy(buffer, "users/mqtt_user/devices/", length - 1);
        strncat(buffer, deviceID.c_str(), length - 1);
        strncat(buffer, "/events/", length - 1);
        strncat(buffer, eventName, length - 1);
        buffer[length - 1] = '\0'; // Ensure null-terminated
    }

    void _getFunctionTopic(char *buffer, size_t length, const char *funcName) {
        strncpy(buffer, "users/mqtt_user/devices/", length - 1);
        strncat(buffer, deviceID.c_str(), length - 1);
        strncat(buffer, "/functions/", length - 1);
        strncat(buffer, funcName, length - 1);
        buffer[length - 1] = '\0'; // Ensure null-terminated
    }

    // Compare a topic with a function name without generating the expected topic
    bool _compareFunctionTopic(const char *topic, const char *funcName) {
        return strncmp(topic, "users/mqtt_user/devices/", 28) == 0 &&
               strncmp(topic + 28, deviceID.c_str(), deviceID.length()) == 0 &&
               strncmp(topic + 28 + deviceID.length(), "/functions/", 11) == 0 &&
               strncmp(topic + 28 + deviceID.length() + 11, funcName, strlen(funcName)) == 0;
    }
};

Fermion Particle;

#endif // wm_fermion_h_
