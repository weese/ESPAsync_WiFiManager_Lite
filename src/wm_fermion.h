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

#define FERMIC_FUNCTION_HANDLERS 10
#define FERMI_CLOUD_MQTT_HOSTNAME "fermicloud.spdns.de"
#define FERMI_CLOUD_MQTT_PORT 8083

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

// Central callback for FermiCloud
void fermionHandlerCallback(char *topic, const char *payload, unsigned int length)
{
    // handle message arrived
    for (uint8_t i = 0; i < Particle.handlerCount; i++)
    {
        FunctionHandler &handler = Particle.handlers[i];
        if (handler.topic == topic)
        {
            if (handler.data)
                handler.funcWithData(handler.data, topic, payload);
            else
                handler.func(topic, payload);
        }
    }
}

// Central callback for FermiCloud MQTT events
static esp_err_t fermionMqttHandler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        Serial.println("MQTT_EVENT_CONNECTED");
        // Subscribe to all handler topics
        for (uint8_t i = 0; i < Particle.handlerCount; i++)
            esp_mqtt_client_subscribe(client, Particle.handlers[i].topic.c_str(), 0);
        
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
        fermionHandlerCallback(event->topic, event->data, event->data_len);
        break;

    case MQTT_EVENT_ERROR:
        Serial.println("MQTT_EVENT_ERROR");
        break;
    }
    return ESP_OK;
}

struct Fermion
{
    FunctionHandler handlers[FERMIC_FUNCTION_HANDLERS];
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
            .event_handle = fermionMqttHandler,
            .uri = FERMI_CLOUD_MQTT_HOSTNAME,
            .port = FERMI_CLOUD_MQTT_PORT,
            .client_id = deviceID.c_str(),
            .username = "",
            .password = accessToken.c_str(),
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
        esp_mqtt_client_subscribe(mqttClient, topic, 0);
        handlers[handlerCount] = {
            .func = handler,
            .topic = topic,
            .data = NULL,
        };
        handlerCount++;
    }

    void unsubscribe(const char *eventName)
    {
        char topic[300];
        _getEventTopic(topic, sizeof(topic), eventName);
        esp_mqtt_client_unsubscribe(mqttClient, topic);
    }

    // bool subscribe(System.deviceID() + "/" PREFIX_WIDGET, handlerWidget, MY_DEVICES);

    // Particle.function("notify", handlerNotificationFunction);

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
