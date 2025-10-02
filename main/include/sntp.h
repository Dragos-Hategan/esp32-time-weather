#ifndef SNTP_H
#define SNTP_H

/**
 * @brief Block until time is synchronized or a timeout occurs.
 *
 * @details
 * Waits for SNTP to complete synchronization. Uses the official
 * esp_netif_sntp_sync_wait() API (ESP-IDF 5.x). If that fails, falls
 * back to a manual check of the system time to validate synchronization.
 *
 * @param timeout_ms Maximum time to wait in milliseconds.
 *
 * @return None
 *
 * @note Logs a warning if synchronization fails within the timeout.
 */
void wait_for_time_blocking(uint32_t timeout_ms);

/**
 * @brief Initialize SNTP and configure the local time zone.
 *
 * @details
 * This function performs the following steps:
 *   - Initializes NVS (required for Wi-Fi).
 *   - Connects to Wi-Fi in station mode.
 *   - Starts SNTP client to synchronize time with NTP servers.
 *   - Configures the timezone for Europe/Bucharest (with automatic DST).
 *   - Blocks until the system time is synchronized or the given timeout expires.
 *
 * @note This function should be called once during system startup,
 *       typically from app_main().
 *
 * @param None
 * @return None
 */
void init_sntp(void);

#endif // SNTP_H