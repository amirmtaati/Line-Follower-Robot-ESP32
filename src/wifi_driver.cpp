#include <Arduino.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <freertos/event_groups.h>
#include "wifi_driver.hh"

static esp_netif_t *s_netif = nullptr;
static EventGroupHandle_t s_eventGroup = nullptr;
static int s_retryCount = 0;
static char s_ipStr[16] = {};
static bool s_connected = false;

static constexpr int MAX_RETRIES = 10;
static constexpr EventBits_t BIT_CONNECTED = BIT0;
static constexpr EventBits_t BIT_FAILED = BIT1;

static void onWiFiEvent(void *, esp_event_base_t base,
                        int32_t id, void *)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        return;
    }

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    {
        s_connected = false;
        if (s_retryCount < MAX_RETRIES)
        {
            s_retryCount++;
            esp_wifi_connect();
        }
        else
        {
            xEventGroupSetBits(s_eventGroup, BIT_FAILED);
        }
        return;
    }

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        esp_netif_ip_info_t info;
        esp_netif_get_ip_info(s_netif, &info);
        snprintf(s_ipStr, sizeof(s_ipStr), IPSTR, IP2STR(&info.ip));
        s_retryCount = 0;
        s_connected = true;
        xEventGroupSetBits(s_eventGroup, BIT_CONNECTED);
    }
}

WiFiResult wifiConnect(const char *ssid, const char *password)
{
    s_eventGroup = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();

    s_netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(s_netif, "line-follower");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = false;
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, onWiFiEvent, nullptr, nullptr);
    esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, onWiFiEvent, nullptr, nullptr);

    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);

    wifi_config_t wifiCfg = {};
    strlcpy(reinterpret_cast<char *>(wifiCfg.sta.ssid),
            ssid, sizeof(wifiCfg.sta.ssid));
    strlcpy(reinterpret_cast<char *>(wifiCfg.sta.password),
            password, sizeof(wifiCfg.sta.password));
    wifiCfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_set_config(WIFI_IF_STA, &wifiCfg);
    esp_wifi_start();

    EventBits_t bits = xEventGroupWaitBits(
        s_eventGroup,
        BIT_CONNECTED | BIT_FAILED,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(20000));

    if (bits & BIT_CONNECTED)
        return WiFiResult::CONNECTED;

    esp_wifi_stop();
    return WiFiResult::FAILED;
}

const char *wifiIPAddress()
{
    return s_ipStr;
}

bool wifiIsConnected()
{
    return s_connected;
}