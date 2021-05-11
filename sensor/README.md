# Hardware
The hardware consists of:

- A base PCB (created in EASYEDA)
- ESP32 Wrover-B
- LIS3DH Accelerometer
- Quectel L80-R
- RFM95W 868 MHz

# Workflow
At the first start the ESP32 configures the Accelermoter. This starts the ESP32 via an interrupt as soon as the sensor is moved (XYZ direction). The ESP32 recognizes the interrupt and starts the GPS receiver. As soon as a fix is available and after a delay of 3 additional seconds, a LoRa message with the quality of the GPS position is sent over LoRaWAN. Afterwards the ESP32, the GPS chipset and the LoRa modem switch to DeepSleep to save power between the wake-up phases.
