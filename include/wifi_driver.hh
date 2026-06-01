#ifndef WIFI_DRIVER_HH
#define WIFI_DRIVER_HH

#include <Arduino.h>
#include <esp_netif.h>

enum class WiFiResult {
    CONNECTED,
    FAILED
};

WiFiResult wifiConnect(const char* ssid, const char* password);

const char* wifiIPAddress();

bool wifiIsConnected();

#endif