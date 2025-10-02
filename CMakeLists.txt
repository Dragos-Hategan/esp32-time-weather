# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/components/bme280-sensor
    ${CMAKE_SOURCE_DIR}/components/ssd1306-oled
    ${CMAKE_SOURCE_DIR}/components/common_i2c
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-time-weather-bme280-ssd1306)
