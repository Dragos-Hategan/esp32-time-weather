/**
 * @file wifi.c
 * @brief Wi-Fi initialization and connection handling for ESP-IDF.
 *
 * @details
 * This module configures the ESP32 Wi-Fi subsystem in station mode (STA),
 * connects to the configured SSID, and blocks execution until a valid
 * IP address is obtained. It also provides automatic reconnection if
 * the connection is lost.
 */

#include "wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "nvs_flash.h"

static const char *TAG_WIFI = "WIFI_INIT";
static SemaphoreHandle_t s_ip_ready;

/**
 * @brief General Wi-Fi and IP event handler.
 *
 * Handles the following cases:
 * - `WIFI_EVENT_STA_START`: Initiates a connection attempt.
 * - `WIFI_EVENT_STA_DISCONNECTED`: Logs a warning and retries connection.
 * - `IP_EVENT_STA_GOT_IP`: Signals that an IP address has been acquired.
 *
 * @param[in] arg        Unused user argument.
 * @param[in] event_base The event base type (Wi-Fi or IP).
 * @param[in] event_id   Specific event ID within the base.
 * @param[in] event_data Pointer to event-specific data (unused here).
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG_WIFI, "Disconnected. Retrying...");
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG_WIFI, "Got IP!");
        if (s_ip_ready) {
            xSemaphoreGive(s_ip_ready);
        }
    }
}

void wifi_init_sta(void)
{
    /* 1) Initialize NVS */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* 2) Network stack + default event loop */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    /* 3) Initialize Wi-Fi driver */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 4) Configure STA parameters */
    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;   /**< Protected Management Frames supported */
    wifi_config.sta.pmf_cfg.required = false; /**< PMF not mandatory */

    /* 5) Create semaphore + register event handlers */
    s_ip_ready = xSemaphoreCreateBinary();
    ESP_ERROR_CHECK(s_ip_ready ? ESP_OK : ESP_ERR_NO_MEM);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    /* 6) Start Wi-Fi + set config */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 7) Block until IP is acquired */
    ESP_LOGI(TAG_WIFI, "Connecting to WiFi...");
    ESP_ERROR_CHECK(xSemaphoreTake(s_ip_ready, portMAX_DELAY) == pdTRUE ? ESP_OK : ESP_FAIL);
    ESP_LOGI(TAG_WIFI, "WiFi connected, proceeding...");
}
