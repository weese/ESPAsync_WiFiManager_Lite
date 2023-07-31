#pragma once

#ifndef wm_wifi_h_
#define wm_wifi_h_

#include <WiFiMulti.h>
#include "wm_config.h"

//////////////////////////////////////////

int _minimumQuality = INT_MIN;

// Scan for WiFiNetworks in range and sort by signal strength
// space for indices array allocated on the heap and should be freed when no longer required
int wmScanWifiNetworks(int **indicesptr)
{
    ESP_WML_LOGDEBUG(F("Scanning WiFis..."));
    int n = WiFi.scanNetworks();
    ESP_WML_LOGDEBUG1(F("scanWifiNetworks: Done, Scanned Networks n = "), n);

    if (n <= 0)
    {
        ESP_WML_LOGDEBUG(F("No network found"));
        return 0;
    }

    // Allocate space off the heap for indices array.
    // This space should be freed when no longer required.
    int *indices = (int *)malloc(n * sizeof(int));
    if (indices == NULL)
    {
        ESP_WML_LOGDEBUG(F("ERROR: Out of memory"));
        *indicesptr = NULL;
        return 0;
    }

    *indicesptr = indices;

    // sort networks
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
    String cssid;

    for (int i = 0; i < n; i++)
    {
        if (indices[i] == -1)
            continue;
        cssid = WiFi.SSID(indices[i]);

        for (int j = i + 1; j < n; j++)
            if (cssid == WiFi.SSID(indices[j]))
            {
                ESP_WML_LOGDEBUG1("DUP AP:", WiFi.SSID(indices[j]));
                indices[j] = -1; // set dup aps to index -1
            }
    }

    for (int i = 0; i < n; i++)
    {
        if (indices[i] == -1)
            continue; // skip dups
        int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));
        if (_minimumQuality > quality)
        {
            indices[i] = -1;
            ESP_WML_LOGDEBUG(F("Skipping low quality"));
        }
    }

    ESP_WML_LOGWARN(F("WiFi networks found:"));

    for (int i = 0; i < n; i++)
    {
        if (indices[i] == -1)
            continue; // skip dups
        ESP_WML_LOGWARN5(i + 1, ": ", WiFi.SSID(indices[i]), ", ", WiFi.RSSI(i), "dB");
    }

    return n;
}

//////////////////////////////////////////

#ifndef WM_HOSTNAME
#define WM_HOSTNAME wmHostname
#endif

String&& wmHostname()
{
    char hostname[15]; // length of "FERMION-XXYYZZ" + 1
    uint8_t eth_mac[6];
    esp_wifi_get_mac((wifi_interface_t)WIFI_IF_STA, eth_mac);
    snprintf(hostname, sizeof(hostname), "FERMION-%02X%02X%02X", eth_mac[3], eth_mac[4], eth_mac[5]);
    return hostname;
}

void wmSetHostname()
{
    String hostname = WM_HOSTNAME();
    ESP_WML_LOGINFO1(F("Hostname="), hostname);
    WiFi.setHostname(hostname.c_str());
}

//////////////////////////////////////////////

// Max times to try WiFi per loop() iteration. To avoid blocking issue in loop()
// Default 1 and minimum 1.
#if !defined(MAX_NUM_WIFI_RECON_TRIES_PER_LOOP)
#define MAX_NUM_WIFI_RECON_TRIES_PER_LOOP 1
#else
#if (MAX_NUM_WIFI_RECON_TRIES_PER_LOOP < 1)
#define MAX_NUM_WIFI_RECON_TRIES_PER_LOOP 1
#endif
#endif

//////////////////////////////////////////

uint8_t wmConnectWifiMulti(WiFiMulti &wifiMulti)
{
    // For ESP32, this better be 0 to shorten the connect time.
    // For ESP32-S2/C3, must be > 500
#if (USING_ESP32_S2 || USING_ESP32_C3)
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 500L
#else
    // For ESP32 core v1.0.6, must be >= 500
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 800L
#endif

#define WIFI_MULTI_CONNECT_WAITING_MS 500L

    ESP_WML_LOGINFO(F("Connecting MultiWifi..."));

    // WiFi.mode(WIFI_STA);

    uint8_t status = wifiMulti.run();
    uint8_t numWiFiReconTries = 0;
    while (status != WL_CONNECTED && numWiFiReconTries++ < MAX_NUM_WIFI_RECON_TRIES_PER_LOOP)
    {
        delay(WIFI_MULTI_CONNECT_WAITING_MS);
        status = WiFi.status();
    }

    if (status == WL_CONNECTED)
    {
        ESP_WML_LOGWARN1(F("WiFi connected after attempts: "), numWiFiReconTries);
        ESP_WML_LOGWARN3(F("SSID="), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
        ESP_WML_LOGWARN3(F("Channel="), WiFi.channel(), F(",IP="), WiFi.localIP());
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

//////////////////////////////////////////

bool wmConnectWifi(WiFiMulti &wifiMulti, WMConfig const &config, wifi_mode_t mode = WIFI_STA)
{
    WiFi.mode(mode);
    for (uint16_t i = 0; i < WM_NUM_WIFI_CREDENTIALS; i++)
        if (config.wifiConfigValidPart(i))
        {
            ESP_WML_LOGDEBUG5(F("bg: addAP : index="), i, F(", SSID="), config.getSSID(i), F(", PWD="), config.getPW(i));
            // WiFi.begin(config.getSSID(i), config.getPW(i));
            // break;
            wifiMulti.addAP(config.getSSID(i), config.getPW(i));
        }
        else
            ESP_WML_LOGWARN3(F("bg: Ignore invalid WiFi PWD : index="), i, F(", PWD="), config.getPW(i));

    if (wmConnectWifiMulti(wifiMulti) != WL_CONNECTED)
    {
        ESP_WML_LOGINFO(F("bg: Fail2connect WiFi"));
        return false;
    }

    ESP_WML_LOGINFO(F("bg: WiFi OK."));
    return true;
}

#endif // wm_wifi_h_