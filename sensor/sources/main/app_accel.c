/*******************************************************************************
 * 
 * Copyright (c) 2021 Hendrik Linka, Hochschule Bonn-Rhein-Sieg
 * 
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 *******************************************************************************/

#include "lis3dh.h"
#include "esp_log.h"

#define SPI_BUS       HSPI_HOST
#define SPI_SCK_GPIO  14
#define SPI_MOSI_GPIO 13
#define SPI_MISO_GPIO 12
#define SPI_CS_GPIO   5
#define INT1_PIN      36
#define INT2_PIN      34

static lis3dh_sensor_t* sensor;
static const char *TAG = "Accel: ";

/**
 * @brief The function configures the accelerometer.
 *
 * Configurate SPI, set values and start tracking motions.
 */

void enable_accelerometer(int threshold){
  spi_bus_init (SPI_BUS, SPI_SCK_GPIO, SPI_MISO_GPIO, SPI_MOSI_GPIO);
  sensor = lis3dh_init_sensor (SPI_BUS, 0, SPI_CS_GPIO);

  if (sensor) {
    ESP_LOGI(TAG, "Lis3dh ok\n");
    lis3dh_int_event_config_t event_config;

    event_config.mode = lis3dh_wake_up;
    event_config.threshold = 10;
    event_config.x_low_enabled  = false;
    event_config.x_high_enabled = true;
    event_config.y_low_enabled  = false;
    event_config.y_high_enabled = true;
    event_config.z_low_enabled  = false;
    event_config.z_high_enabled = true;
    event_config.duration = 0;
    event_config.latch = false;

    lis3dh_set_int_event_config (sensor, &event_config, lis3dh_int_event1_gen);
    lis3dh_enable_int (sensor, lis3dh_int_event1, lis3dh_int1_signal, true);

    lis3dh_config_hpf (sensor, lis3dh_hpf_normal, 0, true, true, true, true);
    lis3dh_get_hpf_ref (sensor);

    //Disalbe pullups
    uint8_t new = 144;
    lis3dh_reg_write (sensor, 0x1e, &new, 1);

    lis3dh_set_scale(sensor, lis3dh_scale_2_g);
    lis3dh_set_mode (sensor, lis3dh_odr_1, lis3dh_low_power, true, true, true);
    lis3dh_remove_spi(SPI_BUS, SPI_CS_GPIO);
  }
  else{
    ESP_LOGI(TAG, "Lis3dh fail\n");
  }
}
