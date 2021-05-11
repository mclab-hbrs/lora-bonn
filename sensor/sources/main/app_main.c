/*******************************************************************************
 * 
 * Copyright (c) 2021 Hendrik Linka, Hochschule Bonn-Rhein-Sieg
 * 
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 *******************************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "app_lora.h"
#include "app_accel.h"
#include "app_gps.h"
#include "driver/spi_master.h"
#include "driver/rtc_io.h"

uint8_t configured;
static const char *TAG = "Main: ";

// Pins and other resources
#define TTN_SPI_HOST      HSPI_HOST
#define TTN_SPI_DMA_CHAN  1
#define TTN_PIN_SPI_SCLK  14
#define TTN_PIN_SPI_MOSI  13
#define TTN_PIN_SPI_MISO  12
#define TTN_PIN_NSS       15
#define TTN_PIN_RXTX      TTN_NOT_CONNECTED
#define TTN_PIN_RST       18
#define TTN_PIN_DIO0      32
#define TTN_PIN_DIO1      33

/**
 * @brief The function restarts the ESP32 if the process takes to long.
 *
 * Restart ESP32, only for safety, should not trigger.
 */
void watch_task(void *pvParameter)
{
	int i=0;
	while(1)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
		if(i>60*10) { //10 minutes
			ESP_LOGI(TAG, "\nForce Reboot!!!\n");
			esp_restart();
		}
		i++;
	}
}

/**
 * @brief The function enables the power source for the gps chipset.
 *
 * Set GPIO_NUM_4 to HIGH to enable GPS power source.
 */
void enable_ldo()
{
	gpio_pad_select_gpio(GPIO_NUM_4);
	gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_4, 1);
}

/**
 * @brief The function is main entry.
 *
 * Check NVS, set the wakeup reasons and disable all chipsets not currently needed. Unfortunate we have to init SPI here for ESP IDF 3.
 */
void app_main()
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	esp_sleep_wakeup_cause_t wakeup_reason;

	xTaskCreate(&watch_task, "watch_task", 2048, NULL, 5, NULL);
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_IO\n"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_CNTL\n"); break;
    case ESP_SLEEP_WAKEUP_TIMER :
			ESP_LOGI(TAG, "Wakeup caused by timer\n");
		  enable_accelerometer(10);
			ESP_LOGI(TAG, "Int sleep\n");
	 		vTaskDelay(1000 / portTICK_RATE_MS);
			esp_sleep_enable_ext0_wakeup(GPIO_NUM_36,ESP_EXT1_WAKEUP_ANY_HIGH);
			esp_deep_sleep_start();
			break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : ESP_LOGI(TAG, "Wakeup caused by touchpad\n"); break;
    case ESP_SLEEP_WAKEUP_ULP : ESP_LOGI(TAG, "Wakeup caused by ULP program\n"); break;
    default :
			ESP_LOGI(TAG, "Wakeup was not caused by deep sleep: %d\n",wakeup_reason);
			disable_ldo();
			disable_lora();
			vTaskDelay(100 / portTICK_PERIOD_MS);
			ESP_LOGI(TAG, "Default sleep\n");
			esp_sleep_enable_timer_wakeup(1000000 * 1); // uS to S * seconds
			esp_deep_sleep_start();
			break;
  }

	enable_ldo();
	ESP_LOGI(TAG, "Enable LDO\n");
	vTaskDelay(100 / portTICK_RATE_MS);

	//We have to init SPI first, else it will fail.
	esp_err_t err;
	gpio_uninstall_isr_service();
	err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	ESP_ERROR_CHECK(err);

	spi_bus_config_t spi_bus_config;
	spi_bus_config.miso_io_num = TTN_PIN_SPI_MISO;
	spi_bus_config.mosi_io_num = TTN_PIN_SPI_MOSI;
	spi_bus_config.sclk_io_num = TTN_PIN_SPI_SCLK;
	spi_bus_config.quadwp_io_num = -1;
	spi_bus_config.quadhd_io_num = -1;
	spi_bus_config.max_transfer_sz = 0;
	err = spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN);
	ESP_ERROR_CHECK(err);

	gps_handler(1000 * 60 * 5); //Timeout in ms
}
