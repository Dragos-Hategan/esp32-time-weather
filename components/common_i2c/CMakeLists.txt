idf_component_register(SRCS "common_i2c_init.c" 
                    INCLUDE_DIRS "include"
                    REQUIRES driver bme280-sensor ssd1306-oled)
