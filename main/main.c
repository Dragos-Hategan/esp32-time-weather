/**
 * @file      main.c
 * @brief     ESP-IDF demo: time + BME280 readings on SSD1306 (I2C), with SNTP and Wi-Fi.
 * @version   1.0.0
 *
 * @details
 * This application:
 *   1) Initializes a shared I2C bus and attaches an SSD1306 OLED and a BME280 sensor
 *   2) Connects to Wi-Fi and starts SNTP to maintain system time
 *   3) Spawns a periodic sensor task that performs a single forced BME280 read every 2.5s
 *   4) Renders current time/date and the latest temperature/pressure/humidity on the OLED each second
 *
 * Concurrency:
 * - A FreeRTOS mutex (s_bme_lock) protects access to the shared BME280 measurement struct.
 *
 * Display:
 * - Text is centered horizontally using the 8x8 font width for layout math.
 * - WIDTH/HEIGHT must match your panel configuration selected in ssd1306 driver.
 *
 * @note Requires working implementations of:
 *       - i2c_shared_init() to create the bus and device handles
 *       - wifi_init_sta() to join a Wi-Fi network
 *       - init_sntp() to start time sync
 *
 * @warning Ensure your SSD1306 WIDTH/HEIGHT and I2C pins match your hardware.
 * @copyright MIT
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/i2c_master.h"

#include "esp_log.h"

#include "fonts.h"
#include "i2c_bus.h"
#include "ssd1306_i2c.h"
#include "ssd1306.h"

#include "bme280_i2c.h"
#include "bme280_defs.h"
#include "config.h"
#include "bme280_read.h"

#include "common_i2c_init.h"
#include "sntp.h"
#include "wifi.h"

static struct bme280_dev bme280_device_handle = {0};
static ssd1306_t ssd1306_device_handle = (ssd1306_t){0};
static struct bme280_data bme280_device_data = {0};

static SemaphoreHandle_t s_bme_lock;

/**
 * @brief Render loop: clears screen, draws time/date and latest BME280 values every second.
 *
 * Centers text horizontally using FONT_8x8 width (8 px).
 * Reads BME280 values under @ref s_bme_lock to avoid tearing.
 *
 * @note Blocks forever; intended to run in the main task context.
 */
static void print_data(void)
{
    ssd1306_clear_screen(&ssd1306_device_handle);

    while (1) {
        time_t now = 0;
        struct tm timeinfo = {0};
        uint16_t horizontal_cursor;
        uint16_t vertical_cursor;

        time(&now);
        localtime_r(&now, &timeinfo);

        // ---- Time (HH:MM:SS), centered on row 1
        char time_buffer[10]; // "HH:MM:SS"
        strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &timeinfo);
        horizontal_cursor = (WIDTH / 2) - (8 * 8 / 2);
        vertical_cursor   = (PIXELS_PER_PAGE * 1) - 4;
        ssd1306_set_cursor(&ssd1306_device_handle, horizontal_cursor, vertical_cursor);
        ssd1306_draw_string(&ssd1306_device_handle, time_buffer, &FONT_8x8, true);

        // ---- Date (YYYY-MM-DD), centered on row 2
        char date_buffer[15]; // "YYYY-MM-DD"
        strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", &timeinfo);
        horizontal_cursor = (WIDTH / 2) - (8 * 10 / 2);
        vertical_cursor   = (PIXELS_PER_PAGE * 2);
        ssd1306_set_cursor(&ssd1306_device_handle, horizontal_cursor, vertical_cursor);
        ssd1306_draw_string(&ssd1306_device_handle, date_buffer, &FONT_8x8, true);

        // ---- Sensor strings (protected read)
        char temperature_str[15];
        char pressure_str[20];
        char humidity_str[15];
        xSemaphoreTake(s_bme_lock, portMAX_DELAY);
        snprintf(temperature_str, sizeof(temperature_str), "Temp-%.1fC",  bme280_device_data.temperature);
        snprintf(pressure_str,    sizeof(pressure_str),    "Pres-%.2fhPa", bme280_device_data.pressure / 100.0);
        snprintf(humidity_str,    sizeof(humidity_str),    "Hum-%.1f%%",   bme280_device_data.humidity);
        xSemaphoreGive(s_bme_lock);

        // ---- Humidity on row 4
        horizontal_cursor = (WIDTH / 2) - ((strlen(humidity_str) * 8) / 2);
        vertical_cursor   = (PIXELS_PER_PAGE * 4) - 4;
        ssd1306_set_cursor(&ssd1306_device_handle, horizontal_cursor, vertical_cursor);
        ssd1306_draw_string(&ssd1306_device_handle, humidity_str, &FONT_8x8, true);

        // ---- Temperature on row 5
        horizontal_cursor = (WIDTH / 2) - ((strlen(temperature_str) * 8) / 2);
        vertical_cursor   = (PIXELS_PER_PAGE * 5) - 2;
        ssd1306_set_cursor(&ssd1306_device_handle, horizontal_cursor, vertical_cursor);
        ssd1306_draw_string(&ssd1306_device_handle, temperature_str, &FONT_8x8, true);

        // ---- Pressure on row 6
        horizontal_cursor = (WIDTH / 2) - ((strlen(pressure_str) * 8) / 2);
        vertical_cursor   = (PIXELS_PER_PAGE * 6);
        ssd1306_set_cursor(&ssd1306_device_handle, horizontal_cursor, vertical_cursor);
        ssd1306_draw_string(&ssd1306_device_handle, pressure_str, &FONT_8x8, true);

        ESP_ERROR_CHECK(ssd1306_update(&ssd1306_device_handle));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Periodically performs a single forced BME280 measurement and publishes it.
 *
 * Uses bme_forced_read_once() to trigger one-shot sampling, then stores the result
 * into @ref bme280_device_data under @ref s_bme_lock.
 *
 * @param[in] arg Unused.
 *
 * @note Runs forever with a 5 s period.
 * @note Stack size and priority are configured in app_main() when creating the task.
 */
void sensor_task(void *arg)
{
    (void)arg;
    
    struct bme280_data tmp;
    while (1)
    {
        bme_forced_read_once(&bme280_device_handle, &tmp);
        xSemaphoreTake(s_bme_lock, portMAX_DELAY);
        bme280_device_data = tmp;                  
        xSemaphoreGive(s_bme_lock);
        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

/**
 * @brief Application entry point.
 *
 * Initializes I2C (shared bus for BME280 + SSD1306), starts Wi-Fi and SNTP,
 * creates the BME mutex, spawns the @ref sensor_task, then enters the render loop.
 *
 * @note Blocks in @ref print_data(); this function never returns.
 */
void app_main(void)
{
    // Initialize shared I2C bus
    ESP_ERROR_CHECK(i2c_shared_init(&bme280_device_handle, &ssd1306_device_handle));
    
    // Initialize Network
    wifi_init_sta();

    // Initialize Time
    init_sntp();

    s_bme_lock = xSemaphoreCreateMutex();
    configASSERT(s_bme_lock);

    xTaskCreatePinnedToCore(sensor_task, "sensor_task", 2 * 1024, NULL, 1, NULL, 1);

    print_data();
}
