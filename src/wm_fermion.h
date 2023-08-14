#pragma once

#ifndef wm_fermion_h_
#define wm_fermion_h_

// #include <Ethernet.h>
#include "mqtt_client.h"
#include "wm_file.h"
#include "wm_wifi.h"

#define WM_TOKEN_FILENAME "/wm_token.dat"
#define WM_TOKEN_FILENAME_BACKUP "/wm_token.bak"

//////////////////////////////////////////////

struct System_t {
    static String deviceID(void) { return "foo"; }
};

static System_t System;

//////////////////////////////////////////////

#define FERMI_CLOUD_FUNCTION_HANDLERS 10
#define FERMI_CLOUD_MQTT_HOSTNAME "fermicloud.spdns.de"
#define FERMI_CLOUD_MQTT_PORT 8083

// This is isrgrootx1.pem, the root Certificate Authority that signed 
// the certifcate of the FermiCloud server https://fermicloud.spdns.de
// This certificate is valid until 04/06/2035, 18:04:38 GMT+7
const char* fermiRootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n" \
"h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n" \
"0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n" \
"A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n" \
"T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n" \
"B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n" \
"B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n" \
"KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n" \
"OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n" \
"jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n" \
"qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n" \
"rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n" \
"HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n" \
"hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n" \
"ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n" \
"3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n" \
"NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n" \
"ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n" \
"TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n" \
"jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n" \
"oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n" \
"4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n" \
"mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n" \
"emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n" \
"-----END CERTIFICATE-----\n";

typedef enum
{
    MY_DEVICES,
    ALL_DEVICES
} FermionScopeEnum;

typedef enum
{
    PRIVATE,
    PUBLIC
} PublishScopeEnum;

typedef void (*EventHandler)(const char *event_name, const char *data);
typedef void (*EventHandlerWithData)(void *handler_data, const char *event_name, const char *data);

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

struct Fermion
{
    FunctionHandler handlers[FERMI_CLOUD_FUNCTION_HANDLERS];
    uint8_t handlerCount;
    esp_mqtt_client_handle_t mqttClient;
    String deviceID;

    Fermion() : 
        handlerCount(0), 
        deviceID(wmHostname()) {}

    bool connect()
    {
        String accessToken;

        // 1. Try to load access token
        if (!loadFile(accessToken, WM_TOKEN_FILENAME) &&
            !loadFile(accessToken, WM_TOKEN_FILENAME_BACKUP)) return false;
        
        // 2. Try to connect to MQTT
        esp_mqtt_client_config_t mqtt_cfg = {
            .event_handle = mqttHandler,
            .uri = FERMI_CLOUD_MQTT_HOSTNAME,
            .port = FERMI_CLOUD_MQTT_PORT,
            .client_id = deviceID.c_str(),
            .username = "",
            .password = accessToken.c_str(),
            .user_context = this,
        };
        mqttClient = esp_mqtt_client_init(&mqtt_cfg);
        if (!mqttClient)
            return false;
        
        esp_mqtt_client_start(mqttClient);
        return true;
    }

    bool publish(const char *eventName, const char *eventData, PublishScopeEnum /*scope*/)
    {
        char topic[300];
        _getEventTopic(topic, sizeof(topic), eventName);
        return esp_mqtt_client_publish(mqttClient, topic, eventData, 0, 0, 0) >= 0;
    }

    bool subscribe(const char *eventName, EventHandler handler, FermionScopeEnum scope)
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

    static esp_err_t mqttHandler(esp_mqtt_event_handle_t event)
    {
        esp_mqtt_client_handle_t client = event->client;
        Fermion *fermion = (Fermion *)(event->user_context);

        switch (event->event_id)
        {
        case MQTT_EVENT_CONNECTED:
            Serial.println("MQTT_EVENT_CONNECTED");
            // Subscribe to all handler topics
            for (uint8_t i = 0; i < fermion->handlerCount; i++)
                esp_mqtt_client_subscribe(client, fermion->handlers[i].topic.c_str(), 0);
            
            break;

        case MQTT_EVENT_DISCONNECTED:
            Serial.println("MQTT_EVENT_DISCONNECTED");
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
            fermion->handlerCallback(event->topic, event->data, event->data_len);
            break;

        case MQTT_EVENT_ERROR:
            Serial.println("MQTT_EVENT_ERROR");
            break;
        }
        return ESP_OK;
    }

private:

    void _getDeviceTopic(char *buffer, size_t length, const char *eventName) {
        strncpy(buffer, "devices/", length);
        strncat(buffer, deviceID.c_str(), length);
        strncat(buffer, "/", length);
        strncat(buffer, eventName, length);
    }

    void _getEventTopic(char *buffer, size_t length, const char *eventName) {
        strncpy(buffer, "devices/events", length);
    }
};

Fermion Particle;

#endif // wm_fermion_h_
