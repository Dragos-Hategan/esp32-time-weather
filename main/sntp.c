#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"

#include "sntp.h"
#include "wifi.h"

static const char *TAG_SNTP = "SNTP";
static const char *TAG_GETT = "GET_TIME";

static const char *EUROPE_ROMANIA_BUCHAREST = "EET-2EEST,M3.5.0/3,M10.5.0/4";

/**
 * @brief Start the SNTP client with a static NTP server.
 *
 * @details
 * Initializes and starts the SNTP service using esp_netif with a fixed
 * NTP server ("pool.ntp.org"). Alternative configuration options include:
 *   - Accepting NTP servers from DHCP.
 *   - Renewing the server list after a new DHCP lease.
 *   - Controlling whether SNTP auto-starts.
 *
 * @note This function should be called once after network initialization
 *       (after Wi-Fi or Ethernet is up).
 *
 * @return None
 */
static void sntp_start(void)
{
    // Simple configuration with static server; see notes above for DHCP options
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    // Example options:
    // config.server_from_dhcp = true;             // accept NTP servers via DHCP
    // config.renew_servers_after_new_IP = true;   // refresh servers on new lease
    // config.start = true;                        // auto-start (default true)

    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));
    ESP_LOGI(TAG_SNTP, "SNTP started via esp_netif");
}

void wait_for_time_blocking(uint32_t timeout_ms)
{
    esp_err_t r = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(timeout_ms));
    if (r == ESP_OK) {
        ESP_LOGI(TAG_GETT, "Time synced");
        return;
    }

    // Very simple fallback check
    for (int retry = 0; retry < 10; ++retry) {
        time_t now = 0;
        struct tm ti = {0};
        time(&now);
        localtime_r(&now, &ti);
        if (ti.tm_year > (2016 - 1900)) {
            ESP_LOGI(TAG_GETT, "Time looks valid (fallback).");
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGW(TAG_GETT, "Time not synced (timeout).");
}


void init_sntp(void)
{
    sntp_start();

    setenv("TZ", EUROPE_ROMANIA_BUCHAREST, 1);
    tzset();

    wait_for_time_blocking(10000);
}