/*******************************************************************************
 *
 * ttn-esp32 - The Things Network device library for ESP-IDF / SX127x
 *
 * Copyright (c) 2018 Manuel Bleichenbacher
 *
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 * Sample program showing how to send a test message every 30 second.
 *******************************************************************************/

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "TheThingsNetwork.h"
#include "../src/lmic/oslmic.h"
#include "../src/lmic/lmic.h"
#include "../src/hal/hal_esp32.h"

// NOTE:
// The LoRaWAN frequency and the radio chip must be configured by running 'make menuconfig'.
// Go to Components / The Things Network, select the appropriate values and save.

// Copy the below hex string from the "Device EUI" field
// on your device's overview page in the TTN console.
const char *devEui = "001AB42A4D74C757";

// Copy the below two lines from bottom of the same page
const char *appEui = "70B3D57ED0028A2F";
const char *appKey = "76D7DC2DD888B3089840F74150AFBCE2";

// Pins and other resources
#define TTN_SPI_HOST      HSPI_HOST
#define TTN_SPI_DMA_CHAN  1
#define TTN_PIN_SPI_SCLK  14
#define TTN_PIN_SPI_MOSI  13
#define TTN_PIN_SPI_MISO  12
#define TTN_PIN_NSS       15
#define TTN_PIN_RXTX      TTN_NOT_CONNECTED
#define TTN_PIN_RST       TTN_NOT_CONNECTED
#define TTN_PIN_DIO0      32
#define TTN_PIN_DIO1      33

static TheThingsNetwork ttn;

const unsigned TX_INTERVAL = 30;
static uint8_t msgData[] = "Hello, world";

RTC_DATA_ATTR bool joined = false;
RTC_DATA_ATTR u4_t RTCnetid;
RTC_DATA_ATTR u4_t RTCdevaddr;
RTC_DATA_ATTR u1_t RTCnwkKey[16];
RTC_DATA_ATTR u1_t RTCartKey[16];
RTC_DATA_ATTR int RTCseqnoUp;
RTC_DATA_ATTR int RTCseqnoDn;



void sendMessages(void* pvParameter)
{
    while (1) {
        printf("Sending message...\n");
        TTNResponseCode res = ttn.transmitMessage(msgData, sizeof(msgData) - 1);
        printf(res == kTTNSuccessfulTransmission ? "Message sent.\n" : "Transmission failed.\n");
        RTCseqnoUp=LMIC.seqnoUp;
		    RTCseqnoDn=LMIC.seqnoDn;
        LMIC_shutdown();
        printf("DeepSleep\n");
        esp_sleep_enable_timer_wakeup(1000000 * 30);


        int64_t time_since_boot = esp_timer_get_time();
        printf("Time since boot: %lld us\n", time_since_boot);
        esp_deep_sleep_start();
    }
}

extern "C" void app_main(void)
{
    esp_err_t err;
    // Initialize the GPIO ISR handler service
    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    ESP_ERROR_CHECK(err);

    // Initialize the NVS (non-volatile storage) for saving and restoring the keys
    err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    // Initialize SPI bus
    spi_bus_config_t spi_bus_config;
    spi_bus_config.miso_io_num = TTN_PIN_SPI_MISO;
    spi_bus_config.mosi_io_num = TTN_PIN_SPI_MOSI;
    spi_bus_config.sclk_io_num = TTN_PIN_SPI_SCLK;
    spi_bus_config.quadwp_io_num = -1;
    spi_bus_config.quadhd_io_num = -1;
    spi_bus_config.max_transfer_sz = 0;
    err = spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN);
    ESP_ERROR_CHECK(err);

    // Configure the SX127x pins
    ttn.configurePins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);

    // The below line can be commented after the first run as the data is saved in NVS
    printf("%s", joined ? "true\n" : "false\n");
    if(!joined){
      ttn.provision(devEui, appEui, appKey);
      printf("Joining...\n");
      if (ttn.join()){
          joined = true;
          printf("Joined.\n");
          LMIC_getSessionKeys (&RTCnetid, &RTCdevaddr,RTCnwkKey,RTCartKey);
          xTaskCreate(sendMessages, "send_messages", 1024 * 4, (void* )0, 3, nullptr);
      }
      else
      {
          printf("Join failed. Goodbye\n");
      }
    }
    else{
      LMIC_reset();
      LMIC_setSession (RTCnetid, RTCdevaddr, RTCnwkKey, RTCartKey);
      LMIC.seqnoUp=RTCseqnoUp;
	    LMIC.seqnoDn=RTCseqnoDn;
      LMIC.adrTxPow = 14;
      LMIC_setLinkCheckMode(0x0);
      LMIC_setDrTxpow(DR_SF7, 14);
      LMIC.datarate = DR_SF7;
      xTaskCreate(sendMessages, "send_messages", 1024 * 4, (void* )0, 3, nullptr);
    }
}
