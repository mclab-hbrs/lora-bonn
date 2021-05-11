/*******************************************************************************
 * 
 * Copyright (c) 2021 Hendrik Linka, Hochschule Bonn-Rhein-Sieg
 * 
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 *******************************************************************************/

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "TheThingsNetwork.h"
#include "../src/lmic/oslmic.h"
#include "../src/lmic/lmic.h"
#include "../src/hal/hal_esp32.h"
#include "../src/lmic/lmic_eu_like.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "LoRa";


static uint8_t NWKSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t APPSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C };
static uint32_t DEVADDR = 0x00000000;

const char *devEui = "0000000000000000";
const char *appEui = "0000000000000000";
const char *appKey = "00000000000000000000000000000000";


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

static TheThingsNetwork ttn;

const unsigned TX_INTERVAL = 30;
uint8_t buffer[25] = {};

int32_t RTCseqnoUp;
int32_t RTCseqnoDn;

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
	vTaskDelay(10 / portTICK_PERIOD_MS);
}

/**
 * @brief The function sends a LoRaWAN message.
 *
 * Send a LoRaWAN message and sleep afterwards.
 */
void sendMessages(void* pvParameter)
{
	while (1) {
		ESP_LOGI(TAG, "Sending message...\n");
		TTNResponseCode res = ttn.transmitMessage(buffer, sizeof(buffer) - 1);
		LMIC_shutdown();
		ESP_LOGI(TAG, "DeepSleep\n");
		vTaskDelay(100 / portTICK_PERIOD_MS);
		esp_sleep_enable_timer_wakeup(1000000 * 120);
		esp_deep_sleep_start();
	}
}

/**
 * @brief The function disables the LoRa modem.
 *
 * Disable the LoRa modem to save power.
 */
extern "C" void disable_lora()
{
	esp_err_t err;
	// Initialize the GPIO ISR handler service
	gpio_uninstall_isr_service();
	err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	ESP_ERROR_CHECK(err);

	// Initialize SPI bus
	spi_bus_config_t spi_bus_config;
	spi_bus_config.miso_io_num = TTN_PIN_SPI_MISO;
	spi_bus_config.mosi_io_num = TTN_PIN_SPI_MOSI;
	spi_bus_config.sclk_io_num = TTN_PIN_SPI_SCLK;
	spi_bus_config.quadwp_io_num = -1;
	spi_bus_config.quadhd_io_num = -1;
	spi_bus_config.max_transfer_sz = 0;
	spi_bus_config.flags = SPICOMMON_BUSFLAG_NATIVE_PINS;
	err = spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN);
	ESP_ERROR_CHECK(err);

	// Configure the SX127x pins
	ttn.configurePins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);
	LMIC_reset();
	LMIC_shutdown();
	ESP_LOGI(TAG, "Disable LoRa\n");
}


/**
 * @brief The function prepares a LoRaWAN message.
 *
 * Prepare a LoRaWAN message and send it to the LoRaWAN message function.
 */
extern "C" void lora_send(float latitude, float longitude, float altitude, float speed, int sats_in_use, float dop_h, int hour, int minute, int second, float thousand)
{
	disable_ldo();
	// Configure the SX127x pins
	ttn.configurePins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);

	// Convert value to bytes
	int32_t lat = latitude * 1e6;
	int32_t lng = longitude * 1e6;
	int16_t alt = altitude * 1e2;
	int32_t sp = speed * 1e6;
	int8_t siu = sats_in_use;
	int32_t dh = dop_h * 1e6;
	int16_t uptime = esp_timer_get_time() / 1e5;
	int8_t hou = hour;
	int8_t min = minute;
	int8_t sec = second;
	int16_t thou = thousand;

	buffer[0] = (lat >> 24) & 0xFF;
	buffer[1] = (lat >> 16) & 0xFF;
	buffer[2] = (lat >> 8) & 0xFF;
	buffer[3] = lat & 0xFF;

	buffer[4] = (lng >> 24) & 0xFF;
	buffer[5] = (lng >> 16) & 0xFF;
	buffer[6] = (lng >> 8) & 0xFF;
	buffer[7] = lng & 0xFF;

	buffer[8] = (alt >> 8) & 0xFF;
	buffer[9] = alt & 0xFF;

	buffer[10] = (sp >> 16) & 0xFF;
	buffer[11] = (sp >> 8) & 0xFF;
	buffer[12] = sp & 0xFF;

	buffer[13] = siu & 0xFF;

	buffer[14] = (dh >> 16) & 0xFF;
	buffer[15] = (dh >> 8) & 0xFF;
	buffer[16] = dh & 0xFF;

	buffer[17] = (uptime >> 8) & 0xFF;
	buffer[18] = uptime & 0xFF;

	buffer[19] = hou & 0xFF;

	buffer[20] = min & 0xFF;

	buffer[21] = sec & 0xFF;

	buffer[22] = (thou >> 8) & 0xFF;
	buffer[23] = thou & 0xFF;

	nvs_handle_t my_handle;
	esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
			ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
	} else {
			RTCseqnoUp = 0;
			RTCseqnoDn = 0;
			nvs_get_i32(my_handle, "RTCseqnoUp", &RTCseqnoUp);
			nvs_get_i32(my_handle, "RTCseqnoDn", &RTCseqnoUp);
			nvs_close(my_handle);
	}
	RTCseqnoUp = RTCseqnoUp + 1;
	RTCseqnoDn = LMIC.seqnoDn;
	err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
	} else {
		err = nvs_set_i32(my_handle, "RTCseqnoUp", RTCseqnoUp);
		err = nvs_commit(my_handle);
		nvs_close(my_handle);
	}

	LMIC_reset();
	LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
	LMIC.adrTxPow = 14;
	LMIC_setLinkCheckMode(0x0);
	LMIC_setAdrMode(0);
	LMIC_setDrTxpow(DR_SF12, 14);
	LMIC.datarate = DR_SF12;
	LMIC.seqnoUp=RTCseqnoUp;
	xTaskCreate(sendMessages, "send_messages", 1024 * 4, (void* )0, 3, nullptr);
}
