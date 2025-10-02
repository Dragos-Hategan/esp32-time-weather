#ifndef COMMON_i2C_INIT_H
#define COMMON_i2C_INIT_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#include "config.h"
#include "ssd1306.h"
#include "bme280_defs.h"

#define COMMON_I2C_PORT         I2C_NUM_0
#define COMMON_I2C_SDA_GPIO     GPIO_NUM_23
#define COMMON_I2C_SCL_GPIO     GPIO_NUM_22

#define CLK_SPEED_100KHZ        100 * 1000
#define CLK_SPEED_400KHZ        400 * 1000

extern const measurement_choice_t bme280_measurement_choice;

/**
 * @brief Initialize the shared I2C bus and both devices (SSD1306 and BME280).
 *
 * @param[in,out] bme280_device_handle Pointer to a BME280 device structure
 *                                     to be initialized.
 * @param[in,out] ssd1306_device_handle Pointer to an SSD1306 device structure
 *                                      to be initialized.
 *
 * @return
 *   - ESP_OK on success  
 *   - Error code from underlying drivers if initialization fails
 *
 * @note This function is idempotent: if the bus is already initialized,
 *       it will return ESP_OK without re-initializing.
 */
esp_err_t i2c_shared_init(struct bme280_dev *bme280_device_handle, ssd1306_t *ssd1306_device_handle);

/**
 * @brief Get the shared I2C bus handle.
 * @return I2C bus handle.
 */
i2c_master_bus_handle_t i2c_get_bus(void);
/**
 * @brief Get the I2C device handle for the BME280 sensor.
 * @return I2C device handle for BME280.
 */
i2c_master_dev_handle_t i2c_get_bme280(void);
/**
 * @brief Get the I2C device handle for the SSD1306 display.
 * @return I2C device handle for SSD1306.
 */
i2c_master_dev_handle_t i2c_get_ssd1306(void);

#endif // COMMON_i2C_INIT_H