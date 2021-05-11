/*******************************************************************************
 * 
 * Copyright (c) 2021 Hendrik Linka, Hochschule Bonn-Rhein-Sieg
 * 
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nmea_parser.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "app_lora.h"

static const char *TAG = "gps";

#define TIME_ZONE (+1)   //Beijing Time
#define YEAR_BASE (2000) //date in GPS starts from 2000
bool fix = false;
int sats_in_view = 0;
gps_t *gps = NULL;
float d_hop_limit = 1.7;

float lat;
float lng;
float alt;
float sp;
int siu;
float dh;
int hour;
int minute;
int second;
float thousand;
int delay = 0;

/**
 * @brief The function handles incoming GPS messages.
 *
 * Check for an fix of the gps chipset and set all information about the quality of the fix after a very short delay.
 */

static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

  switch (event_id) {
  case GPS_UPDATE:
    gps = (gps_t *)event_data;
    dh = gps->dop_h;
    sats_in_view = gps->sats_in_view;
    if(gps->fix) {
      delay = delay + 1;
    }
    if(gps->fix && delay > 3) {
      lat = gps->latitude;
      lng = gps->longitude;
      alt = gps->altitude;
      sp = gps->speed;
      siu = gps->sats_in_use;
      hour = gps->tim.hour;
      minute = gps->tim.minute;
      second = gps->tim.second;
      thousand = gps->tim.thousand;
      fix = true;
    }
    break;
  case GPS_UNKNOWN:
      ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
    break;
  default:
    break;
  }
}

/**
 * @brief The function disables the power source.
 *
 * Disable the power source of the gps chipset.
 */

void disable_ldo()
{
  gpio_pad_select_gpio(GPIO_NUM_4);
  gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_4, 0);
}

/**
 * @brief The function controls the state of the gps measurement
 *
 * Start searching for a fix for a finite time. Deactivate the chipset if no fix is found in a finite time.
 */

bool gps_handler(int timeout_msec)
{
  nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
  nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
  nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);
  int time = 0;

  while(time < timeout_msec) {
    if(!fix) {
      vTaskDelay(200 / portTICK_PERIOD_MS);
      time = time + 200;
    }
    else{
      disable_ldo();
      vTaskDelay(100 / portTICK_PERIOD_MS);
      lora_send(lat,lng,alt,sp,siu,dh,hour,minute,second,thousand);
      nmea_parser_remove_handler(nmea_hdl, gps_event_handler);
      nmea_parser_deinit(nmea_hdl);
      return true;
    }
  }
  ESP_LOGI(TAG, "DeepSleep\n");
  if(sats_in_view<3) {
    //Probably we are indoor!
    ESP_LOGI(TAG, "Indoor\n");
    disable_ldo();
    esp_sleep_enable_timer_wakeup(1000000 * 120);
  }
  else{
    ESP_LOGI(TAG, "Try again later\n");
    disable_ldo();
    esp_sleep_enable_timer_wakeup(1000000 * 120);
  }
  vTaskDelay(100 / portTICK_PERIOD_MS);
  esp_deep_sleep_start();
  return false;
}
