/**
 * @file common_i2c_init.c
 * @brief Shared I2C bus initialization and device handles.
 *
 * @details
 * This module initializes a shared I2C bus and attaches two devices:
 *   - SSD1306 OLED display
 *   - BME280 environmental sensor
 *
 * It exposes accessors for the bus handle and the two device handles
 * so they can be used in other modules without re-initializing the bus.
 */

#include "common_i2c_init.h"
#include "bme280_defs.h"
#include "bme280_config.h"
#include "ssd1306.h"
#include "i2c_bus.h"

#include "ssd1306_font8x8.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

static const char* TAG = "COMMON_I2C";

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t sensor_i2c_dev;
static i2c_master_dev_handle_t screen_i2c_dev;

const measurement_choice_t bme280_measurement_choice = FORCED_PERIODIC_ONE_TIME;

/**
 * @brief Initialize the BME280 sensor on the shared I2C bus.
 *
 * @param bme280_device_handle Pointer to a BME280 device structure to initialize.
 *
 * @note This function:
 *   - Gets the I2C device handle from the shared bus.
 *   - Calls the low-level initialization routine.
 *   - Configures sensor settings according to the selected measurement mode.
 *
 * @warning Logs an error if sensor configuration fails.
 */
static void init_sensor(struct bme280_dev *bme280_device_handle)
{
    i2c_master_dev_handle_t sensor_i2c_dev = i2c_get_bme280();
    bme280_device_init(bme280_device_handle, sensor_i2c_dev);
    int8_t rslt = configure_sensor_settings(bme280_measurement_choice, bme280_device_handle);
    if (rslt != BME280_OK){ ESP_LOGE("BME280", "configure_sensor_settings failed: %d", rslt);}
}

/**
 * @brief Initialize the SSD1306 OLED screen on the shared I2C bus.
 *
 * @param ssd1306_device_handle Pointer to an SSD1306 driver instance to initialize.
 *
 * @note This function:
 *   - Builds bus and device wrappers from the shared I2C bus.
 *   - Probes the SSD1306 at the default address.
 *   - Binds the device to the SSD1306 driver.
 *   - Initializes the display with configured dimensions and charge pump.
 *   - Shows an initial message 
 *
 * @warning Will raise an error and abort execution if the device is not detected.
 */
static void init_screen(ssd1306_t *ssd1306_device_handle)
{
    i2c_bus_t ssd1306_bus = {
        .bus = i2c_get_bus()
    };

    i2c_device_t screen_dev = {
        .dev = i2c_get_ssd1306(),
        .scl_speed_hz = CLK_SPEED_400KHZ,
        .addr7 = SSD1306_I2C_ADDR_DEFAULT
    };

    ESP_ERROR_CHECK(i2c_device_probe(&ssd1306_bus, SSD1306_I2C_ADDR_DEFAULT, I2C_TIMEOUT_MS));
    ESP_LOGI("SSD1306", "SSD1306 detected at 0x%02X", SSD1306_I2C_ADDR_DEFAULT);

    ssd1306_link_t link = (ssd1306_link_t){0};
    ssd1306_link_from_device(&link, &screen_dev);

    ESP_ERROR_CHECK(ssd1306_init(ssd1306_device_handle, &link, WIDTH, HEIGHT, /*external_vcc=*/false));    

    char init_message[] = "Getting Data";
    uint16_t horisontal_cursor = (WIDTH / 2) - ((sizeof(init_message) / 2) * 8);
    uint16_t vertical_cursor = ((HEIGHT / 8) * 2);
    ssd1306_set_cursor(ssd1306_device_handle, horisontal_cursor, vertical_cursor);
    ssd1306_draw_string(ssd1306_device_handle, init_message, &FONT_8x8, true);
    ESP_ERROR_CHECK(ssd1306_update(ssd1306_device_handle));
}

esp_err_t i2c_shared_init(struct bme280_dev *bme280_device_handle, ssd1306_t *ssd1306_device_handle) {
    if (i2c_bus) return ESP_OK;

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = COMMON_I2C_PORT,
        .sda_io_num = COMMON_I2C_SDA_GPIO,
        .scl_io_num = COMMON_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &i2c_bus), TAG, "bus fail");

    i2c_device_config_t ssd_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SSD1306_I2C_ADDR_DEFAULT,
        .scl_speed_hz = CLK_SPEED_400KHZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(i2c_bus, &ssd_cfg, &screen_i2c_dev), TAG, "ssd fail");

    i2c_device_config_t bme_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME280_I2C_ADDR_PRIM, /**< or 0x77 */
        .scl_speed_hz = CLK_SPEED_400KHZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(i2c_bus, &bme_cfg, &sensor_i2c_dev), TAG, "bme fail");

    init_sensor(bme280_device_handle);
    init_screen(ssd1306_device_handle);

    return ESP_OK;
}

i2c_master_bus_handle_t i2c_get_bus(void) { return i2c_bus; }
i2c_master_dev_handle_t i2c_get_bme280(void) { return sensor_i2c_dev; }
i2c_master_dev_handle_t i2c_get_ssd1306(void) { return screen_i2c_dev; }
