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

const char* FERMI_CLOUD_TOKEN_URL            PROGMEM = "https://fermicloud.dev/auth/realms/fermi-cloud/protocol/openid-connect/token";
const char* FERMI_CLOUD_USERINFO_URL         PROGMEM = "https://fermicloud.dev/auth/realms/fermi-cloud/protocol/openid-connect/userinfo";
const char* FERMI_CLOUD_DEVICE_CODE_PAYLOAD  PROGMEM = "client_id=fermi-device&scope=openid&grant_type=urn:ietf:params:oauth:grant-type:device_code&device_code=";
const char* FERMI_CLOUD_REFRESH_PAYLOAD      PROGMEM = "client_id=fermi-device&grant_type=refresh_token&refresh_token=";
#define FERMI_VERSION "1.0.0"
#define FERMI_DEVICE "ESP32"

//////////////////////////////////////////////

struct System_t {
    static String deviceID(void) { return "foo"; }
};

static System_t System;

//////////////////////////////////////////////

#define FERMI_CLOUD_EVENT_HANDLERS          20
#define FERMI_CLOUD_FUNCTION_HANDLERS       20
#define FERMI_CLOUD_VARIABLES               20

#define FERMI_CLOUD_DEVICE_ID_LENGTH        32
#define FERMI_CLOUD_EVENT_NAME_LENGTH       64
#define FERMI_CLOUD_FUNCTION_NAME_LENGTH    64
#define FERMI_CLOUD_VARIABLE_NAME_LENGTH    64


#define FERMI_CLOUD_BASE_BUFFER_LENGTH           (sizeof("devices/") + FERMI_CLOUD_DEVICE_ID_LENGTH + 1)
#define FERMI_CLOUD_EVENT_TOPIC_BUFFER_LENGTH    (FERMI_CLOUD_BASE_BUFFER_LENGTH + sizeof("events/") + FERMI_CLOUD_EVENT_NAME_LENGTH + 1)
#define FERMI_CLOUD_FUNCTION_TOPIC_BUFFER_LENGTH (FERMI_CLOUD_BASE_BUFFER_LENGTH + sizeof("functions/") + FERMI_CLOUD_FUNCTION_NAME_LENGTH + 1)
#define FERMI_CLOUD_VARIABLE_TOPIC_BUFFER_LENGTH (FERMI_CLOUD_BASE_BUFFER_LENGTH + sizeof("variables/") + FERMI_CLOUD_VARIABLE_NAME_LENGTH + 1)

#define FERMI_CLOUD_MQTT_URI            "wss://fermicloud.dev:8084/mqtt"
// #define FERMI_CLOUD_MQTT_URI "mqtts://fermicloud.dev:8883"

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
"MIIEVzCCAj+gAwIBAgIRAIOPbGPOsTmMYgZigxXJ/d4wDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw\n" \
"WhcNMjcwMzEyMjM1OTU5WjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\n" \
"RW5jcnlwdDELMAkGA1UEAxMCRTUwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAQNCzqK\n" \
"a2GOtu/cX1jnxkJFVKtj9mZhSAouWXW0gQI3ULc/FnncmOyhKJdyIBwsz9V8UiBO\n" \
"VHhbhBRrwJCuhezAUUE8Wod/Bk3U/mDR+mwt4X2VEIiiCFQPmRpM5uoKrNijgfgw\n" \
"gfUwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcD\n" \
"ATASBgNVHRMBAf8ECDAGAQH/AgEAMB0GA1UdDgQWBBSfK1/PPCFPnQS37SssxMZw\n" \
"i9LXDTAfBgNVHSMEGDAWgBR5tFnme7bl5AFzgAiIyBpY9umbbjAyBggrBgEFBQcB\n" \
"AQQmMCQwIgYIKwYBBQUHMAKGFmh0dHA6Ly94MS5pLmxlbmNyLm9yZy8wEwYDVR0g\n" \
"BAwwCjAIBgZngQwBAgEwJwYDVR0fBCAwHjAcoBqgGIYWaHR0cDovL3gxLmMubGVu\n" \
"Y3Iub3JnLzANBgkqhkiG9w0BAQsFAAOCAgEAH3KdNEVCQdqk0LKyuNImTKdRJY1C\n" \
"2uw2SJajuhqkyGPY8C+zzsufZ+mgnhnq1A2KVQOSykOEnUbx1cy637rBAihx97r+\n" \
"bcwbZM6sTDIaEriR/PLk6LKs9Be0uoVxgOKDcpG9svD33J+G9Lcfv1K9luDmSTgG\n" \
"6XNFIN5vfI5gs/lMPyojEMdIzK9blcl2/1vKxO8WGCcjvsQ1nJ/Pwt8LQZBfOFyV\n" \
"XP8ubAp/au3dc4EKWG9MO5zcx1qT9+NXRGdVWxGvmBFRAajciMfXME1ZuGmk3/GO\n" \
"koAM7ZkjZmleyokP1LGzmfJcUd9s7eeu1/9/eg5XlXd/55GtYjAM+C4DG5i7eaNq\n" \
"cm2F+yxYIPt6cbbtYVNJCGfHWqHEQ4FYStUyFnv8sjyqU8ypgZaNJ9aVcWSICLOI\n" \
"E1/Qv/7oKsnZCWJ926wU6RqG1OYPGOi1zuABhLw61cuPVDT28nQS/e6z95cJXq0e\n" \
"K1BcaJ6fJZsmbjRgD5p3mvEf5vdQM7MCEvU0tHbsx2I5mHHJoABHb8KVBgWp/lcX\n" \
"GWiWaeOyB7RP+OfDtvi2OsapxXiV7vNVs7fMlrRjY1joKaqmmycnBvAq14AEbtyL\n" \
"sVfOS66B8apkeFX2NY4XPEYV4ZSCe8VHPrdrERk2wILG3T/EGmSIkCYVUMSnjmJd\n" \
"VQD9F6Na/+zmXCc=\n" \
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

typedef void (*EventHandlerFunction)(const char *event_name, const char *data);
typedef void (*EventHandlerFunctionWithData)(void *handler_data, const char *event_name, const char *data);
typedef int (*CloudFunctionHandler)(String);
typedef void (*CloudVariableSerializer)(JsonDocument &doc, const void *ref);

struct CloudEventHandler
{
    String topic;
    union
    {
        EventHandlerFunction func;
        EventHandlerFunctionWithData funcWithData;
    };
    void *data;
};

struct CloudFunction
{
    String funcKey;
    CloudFunctionHandler func;
};

// Structure for function request/response
struct CloudFunctionRequest {
    uint32_t requestId;
    String parameters;
};

struct CloudVariable {
    // name of the variable
    String name;
    // function to serialize the variable
    CloudVariableSerializer serializer;
    // reference to the variable
    const void *ref;
};

class FermiDevice {
public:
    CloudEventHandler eventHandlers[FERMI_CLOUD_EVENT_HANDLERS];
    CloudFunction functionHandlers[FERMI_CLOUD_FUNCTION_HANDLERS];
    CloudVariable variables[FERMI_CLOUD_VARIABLES];
    uint8_t eventHandlerCount;
    uint8_t functionHandlerCount;
    uint8_t variableCount;
    bool _gotDisconnected;
    bool _isConnected;
    esp_mqtt_client_handle_t mqttClient;
    String deviceID;
    String accessToken;


    FermiDevice() : 
        eventHandlerCount(0),
        functionHandlerCount(0),
        variableCount(0),
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
    
    ~FermiDevice() {
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

    String fetchUsername() {
        WiFiClientSecure client;
        client.setCACert(fermiRootCACertificate);
        {
            HTTPClient https;
            https.setConnectTimeout(5000);
            https.setTimeout(5000);
            https.begin(client, FERMI_CLOUD_USERINFO_URL);
            https.addHeader("Authorization", "Bearer " + accessToken);
            
            int httpCode = https.GET();
            ESP_WML_LOGINFO1(F("s:Userinfo status code = "), httpCode);
            
            String username;
            if (httpCode == 200) {
                StaticJsonDocument<512> doc;
                DeserializationError error = deserializeJson(doc, https.getStream());
                if (!error) {
                    username = doc["username"].as<const char*>();
                    ESP_WML_LOGINFO1(F("s:Username = "), username.c_str());
                } else {
                    ESP_WML_LOGINFO1(F("s:Userinfo deserialisation error: "), error.c_str());
                }
            } else {
                ESP_WML_LOGINFO1(F("s:Userinfo request failed with code: "), httpCode);
            }
            https.end();
            return username;
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

            // Fetch username from userinfo endpoint
            String username = fetchUsername();
            if (username.length() == 0) {
                return false;
            }

            esp_mqtt_client_config_t mqtt_cfg = {
                .uri = FERMI_CLOUD_MQTT_URI,
                .client_id = deviceID.c_str(),
                .username = username.c_str(),
                .password = accessToken.c_str(),
                .cert_pem = fermiRootCACertificate,
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
            
            publishCapabilities();

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
        if (strlen(eventName) > FERMI_CLOUD_EVENT_NAME_LENGTH) {
            ESP_WML_LOGERROR(F("s:Event name too long"));
            return -1;
        }
        char topic[FERMI_CLOUD_EVENT_TOPIC_BUFFER_LENGTH];
        _getEventTopic(topic, sizeof(topic), eventName);
        return esp_mqtt_client_publish(mqttClient, topic, eventData, 0, 0, flags & WITH_RETAIN ? 1 : 0);
    }

    bool subscribe(const char *eventName, EventHandlerFunction handler, SubscribeScopeEnum scope)
    {
        char topic[FERMI_CLOUD_EVENT_TOPIC_BUFFER_LENGTH];
        _getEventTopic(topic, sizeof(topic), eventName);
        if (esp_mqtt_client_subscribe(mqttClient, topic, 0) < 0)
            return false;
        eventHandlers[eventHandlerCount] = CloudEventHandler{
            .topic = topic,
            .func = handler,
            .data = NULL,
        };
        eventHandlerCount++;
        return true;
    }

    bool unsubscribe(const char *eventName)
    {
        char topic[FERMI_CLOUD_EVENT_TOPIC_BUFFER_LENGTH];
        _getEventTopic(topic, sizeof(topic), eventName);
        return esp_mqtt_client_unsubscribe(mqttClient, topic) >= 0;
    }

    // Add a cloud function
    bool function(const char *funcKey, CloudFunctionHandler func)
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
        
        functionHandlers[functionHandlerCount] = CloudFunction{
            .funcKey = String(funcKey),
            .func = func
        };
        functionHandlerCount++;
        
        ESP_WML_LOGINFO1(F("s:Function registered: "), funcKey);
        return true;
    }

    // bool subscribe(System.deviceID() + "/" PREFIX_WIDGET, handlerWidget, MY_DEVICES);

    // Particle.function("notify", handlerNotificationFunction);

    // Add a cloud variable
    template <typename T>
    bool variable(const char *name, const T &ref)
    {
        return _variable(name, [](JsonDocument &doc, const void *ref) {
            doc["v"] = *(const T *)ref;
        }, &ref);
    }

    // Add a calculated cloud variable
    template <typename T>
    bool variable(const char *name, T (*func)())
    {
        return _variable(name, [](JsonDocument &doc, const void *ref) {
            doc["v"] = ((T (*)()) ref)();
        }, (void *) func);
    }

    bool _variable(const char *name, CloudVariableSerializer serializer, const void *ref)
    {
        // Check if variable with same name already exists
        for (uint8_t i = 0; i < variableCount; i++) {
            if (variables[i].name == name) {
                // Replace existing variable
                variables[i].serializer = serializer;
                variables[i].ref = ref;
                ESP_WML_LOGINFO1(F("s:Variable replaced: "), name);
                return true;
            }
        }
        
        // Add new function if we have space
        if (variableCount >= FERMI_CLOUD_VARIABLES) {
            ESP_WML_LOGERROR(F("s:Too many variables registered"));
            return false;
        }
        
        variables[variableCount] = {
            .name = String(name),
            .serializer = serializer,
            .ref = ref,
        };
        variableCount++;
        return true;
    }

    bool publishCapabilities() 
    {
        StaticJsonDocument<2048> doc;

        // Iterate over all functionHandler funcKeys and append them to the funcs array
        {
            JsonArray funcs = doc["f"].to<JsonArray>();        
            for (uint8_t i = 0; i < functionHandlerCount; i++) {
                funcs.add(functionHandlers[i].funcKey.c_str());
            }
        }

        // Iterate over all variable names and append them to the vars array
        {
            JsonArray vars = doc["v"].to<JsonArray>();        
            for (uint8_t i = 0; i < variableCount; i++) {
                vars.add(variables[i].name.c_str());
            }
        }

        String payload;
        serializeJson(doc, payload);
        char topic[FERMI_CLOUD_BASE_BUFFER_LENGTH + sizeof("capabilities") + 1];
        _getDeviceTopic(topic, sizeof(topic), "capabilities");
        return esp_mqtt_client_publish(mqttClient, topic, payload.c_str(), 0, 1, 1) != -1;
    }

    void eventCallback(const char *topic, size_t topic_length, const char *payload, size_t length)
    {
        for (uint8_t i = 0; i < eventHandlerCount; i++)
        {
            CloudEventHandler &handler = eventHandlers[i];
            if (handler.topic.length() == topic_length && strncmp(topic, handler.topic.c_str(), topic_length) == 0)
            {
                if (handler.data)
                    handler.funcWithData(handler.data, topic, payload);
                else
                    handler.func(topic, payload);
            }
        }
    }

    void functionCallback(const char *topic, size_t topic_len, const char *payload, size_t length)
    {
        for (uint8_t i = 0; i < functionHandlerCount; i++)
        {
            CloudFunction &handler = functionHandlers[i];
            if (handler.funcKey.length() == topic_len && strncmp(handler.funcKey.c_str(), topic, topic_len) == 0) {
                // Parse the request payload
                String payloadStr(payload, length);
                CloudFunctionRequest request;
                
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
                
                String responseJson;
                {
                    // Create JSON response
                    StaticJsonDocument<256> responseDoc;
                    responseDoc["i"] = request.requestId;
                    responseDoc["r"] = result;
                    
                    serializeJson(responseDoc, responseJson);
                }
                
                // Send function result back
                char responseTopic[FERMI_CLOUD_FUNCTION_TOPIC_BUFFER_LENGTH + sizeof("/response")];
                snprintf(responseTopic, sizeof(responseTopic), "devices/%s/functions/%s/response", deviceID.c_str(), handler.funcKey.c_str());
                esp_mqtt_client_publish(mqttClient, responseTopic, responseJson.c_str(), 0, 0, 0);
            }
        }
    }

    void variableCallback(const char *topic, size_t topic_len)
    {
        for (uint8_t i = 0; i < variableCount; i++) {
            CloudVariable &variable = variables[i];
            if (variable.name.length() == topic_len && strncmp(variable.name.c_str(), topic, topic_len) == 0) {
                String responseJson;
                {
                    // Create JSON response
                    StaticJsonDocument<256> responseDoc;
                    variable.serializer(responseDoc, variable.ref);
                    serializeJson(responseDoc, responseJson);
                }
                                
                // Send variable value back
                char responseTopic[FERMI_CLOUD_VARIABLE_TOPIC_BUFFER_LENGTH + sizeof("/value")];
                snprintf(responseTopic, sizeof(responseTopic), "devices/%s/variables/%s/value", deviceID.c_str(), variable.name.c_str());
                esp_mqtt_client_publish(mqttClient, responseTopic, responseJson.c_str(), 0, 0, 0);
            }
        }
    }

    static void mqttHandlerStatic(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
        ((FermiDevice *)handler_args)->mqttHandler(base, event_id, event_data);
    }

    esp_err_t mqttHandler(esp_event_base_t base, int32_t event_id, void *event_data)
    {
        Serial.printf("Event dispatched from event loop base=%s, event_id=%d", base, event_id);
        esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
        esp_mqtt_client_handle_t client = event->client;
        const char* topic = event->topic;
        size_t topic_len = event->topic_len;
        int msg_id;

        switch (event_id)
        {
        case MQTT_EVENT_CONNECTED:
            Serial.println("MQTT_EVENT_CONNECTED");
            _isConnected = true;

            // Subscribe to all handler topics
            for (uint8_t i = 0; i < eventHandlerCount; i++)
                esp_mqtt_client_subscribe(client, eventHandlers[i].topic.c_str(), 0);

            // Subscribe to all function topics
            {
                char topic[FERMI_CLOUD_BASE_BUFFER_LENGTH + sizeof("functions/#") + 1];
                _getDeviceTopic(topic, sizeof(topic), "functions/#");
                esp_mqtt_client_subscribe(mqttClient, topic, 0);
            }

            // Subscribe to all variable topics
            {
                char topic[FERMI_CLOUD_BASE_BUFFER_LENGTH + sizeof("variables/#") + 1];
                _getDeviceTopic(topic, sizeof(topic), "variables/#");
                esp_mqtt_client_subscribe(mqttClient, topic, 0);
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
            Serial.printf("TOPIC=%.*s\n", event->topic_len, event->topic);
            Serial.printf("DATA=%.*s\n", event->data_len, event->data);

            // Check if the topic is for this device
            if (strncmp(topic, "devices/", 8) != 0) break;
            topic += 8;
            topic_len -= 8;
            if (strncmp(topic, deviceID.c_str(), deviceID.length()) != 0) break;
            topic += deviceID.length();
            topic_len -= deviceID.length();

            // Check if the topic is a function or event
            if (strncmp(topic, "/events/", 8) == 0) {
                topic += 8;
                topic_len -= 8;
                eventCallback(topic, topic_len, event->data, event->data_len);
            } else if (strncmp(topic, "/functions/", 11) == 0) {
                topic += 11;
                topic_len -= 11;
                functionCallback(topic, topic_len, event->data, event->data_len);
            } else if (strncmp(topic, "/variables/", 11) == 0) {
                topic += 11;
                topic_len -= 11;
                variableCallback(topic, topic_len);
            }
            break;

        case MQTT_EVENT_ERROR:
            Serial.println("MQTT_EVENT_ERROR");
            break;
        }
        return ESP_OK;
    }

private:

    void _getDeviceTopic(char *buffer, size_t length, const char *subTopic) {
        strncpy(buffer, "devices/", length - 1);
        strncat(buffer, deviceID.c_str(), length - 1);
        strncat(buffer, "/", length - 1);
        strncat(buffer, subTopic, length - 1);
        buffer[length - 1] = '\0'; // Ensure null-terminated
    }

    void _getEventTopic(char *buffer, size_t length, const char *eventName) {
        strncpy(buffer, "devices/", length - 1);
        strncat(buffer, deviceID.c_str(), length - 1);
        strncat(buffer, "/events/", length - 1);
        strncat(buffer, eventName, length - 1);
        buffer[length - 1] = '\0'; // Ensure null-terminated
    }
};

FermiDevice Fermion;
FermiDevice &Particle = Fermion;    // alias for Fermion for easier migration

#endif // wm_fermion_h_
