# ESP32 Time + Weather (SSD1306, BME280, SNTP, Wi-Fi) — ESP-IDF

> **Why this repo?**  
Minimal, production-minded example that shows real-time clock via SNTP and BME280 readings rendered to an SSD1306 OLED over the new ESP-IDF i2c_master_* API. Concise modules, clear concurrency, and sensible defaults.

## Features

- New I²C master driver (driver/i2c_master.h, ESP-IDF 5.x+)
- SSD1306 text rendering (shadow buffer + monospace fonts)
- Bosch BME280 temperature / pressure / humidity (forced one-shot loop)
- Wi-Fi STA with blocking connect (waits for valid IP)
- SNTP setup with timezone (Europe/Bucharest by default)
- Thread-safe sensor data via FreeRTOS mutex
- Simple layout math (8×8 font, centered strings)

## Hardware

- ESP32 dev board (ESP-WROOM-32 class; ESP-IDF 5.x)
- SSD1306 OLED 128×64 (I²C, addr 0x3C/0x3D)
- BME280 (I²C, addr 0x76/0x77)
- Default pins (changeable in headers):
- SDA = GPIO 23, SCL = GPIO 22
- I²C clock 400 kHz
- Pull-ups 4.7–10 kΩ recommended for longer wires

## API overview
### Wi-Fi (wifi.h)
- void wifi_init_sta(void);
Initializes NVS, netif, event loop, registers handlers, sets STA config from config.h, starts Wi-Fi, and blocks until IP_EVENT_STA_GOT_IP.  
  
Events handled
- WIFI_EVENT_STA_START → connect
- WIFI_EVENT_STA_DISCONNECTED → log + reconnect
- IP_EVENT_STA_GOT_IP → releases semaphore

### SNTP (sntp.h)
- void init_sntp(void);
Starts SNTP (static server "pool.ntp.org" by default), applies TZ (Europe/Bucharest), then wait_for_time_blocking(10000).
- void wait_for_time_blocking(uint32_t timeout_ms);
Uses esp_netif_sntp_sync_wait(). Fallback: checks tm_year to avoid false negatives.

### Display (ssd1306.h)
- Lifecycle: ssd1306_init, ssd1306_deinit
- Power/Mode: ssd1306_display_on/off, ssd1306_set_normal/inverse
- Buffer: ssd1306_clear, ssd1306_clear_screen, ssd1306_update
- Text: ssd1306_set_cursor, ssd1306_draw_string (uses FONT_8x8)
- I²C link: ssd1306_link_from_device, ssd1306_cmdN, ssd1306_data  
print_data() composes the UI:
- Row 1: time (HH:MM:SS)
- Row 2: date (YYYY-MM-DD)
- Rows 4–6: Hum / Temp / Pres (centered)
- 
### BME280 (bme280_read.h, bme280_i2c.h)
- bme_forced_read_once(struct bme280_dev *dev, struct bme280_data *out);  
Triggers one conversion in FORCED mode; returns temperature (°C), pressure (Pa), humidity (%).

## Notes & tips

- **Production error handling**  
Keep ESP_ERROR_CHECK() for critical init (I²C/SSD1306/BME bring-up).
At runtime, prefer ESP_RETURN_ON_ERROR() + retries/recovery for transient I²C timeouts or sensor hiccups to avoid panic-reboots.
- **I²C stability**  
If you see sporadic NACKs: shorten wires, add proper pull-ups, try 100 kHz, or tune glitch filters if your wrapper supports it.
- **Centering math**  
With FONT_8x8, horizontal center is:
> x = (WIDTH/2) - ((strlen(text) * 8) / 2)
- **Pressure units*  
BME280 raw pressure is in Pa; divide by 100.0 for hPa.
