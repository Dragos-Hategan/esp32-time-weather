idf_component_register(
        SRCS "main.c" "wifi.c" "sntp.c"
        INCLUDE_DIRS "include"
        REQUIRES 
                bme280-sensor 
                ssd1306-oled
                common_i2c
                nvs_flash
                esp_wifi
                esp_event
                esp_netif
                lwip
)
