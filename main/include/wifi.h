#ifndef WIFI_H
#define WIFI_H

#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASS "WIFI_PASS"

/**
 * @brief Initialize Wi-Fi in STA mode and wait for IP.
 *
 * This function:
 * 1. Initializes NVS storage (required for Wi-Fi).
 * 2. Initializes the TCP/IP network stack and default event loop.
 * 3. Creates the default Wi-Fi STA network interface.
 * 4. Configures Wi-Fi parameters (SSID, password, auth mode, PMF).
 * 5. Registers event handlers for Wi-Fi and IP events.
 * 6. Starts the Wi-Fi driver and connects.
 * 7. Blocks execution until an IP address is successfully obtained.
 *
 * @note This function blocks indefinitely until an IP is acquired.
 *       Automatic reconnection is handled in the event handler.
 */
void wifi_init_sta(void);

#endif // WIFI_H