# Gateway

## TODO ![](https://img.shields.io/badge/status-todo-red)

- Verify Gateway watchdog (insert the WDT in parse function) ![](https://img.shields.io/badge/status-todo-red)
- Weather data received semantic analysis ![](https://img.shields.io/badge/status-todo-red)
- onboard LED blink until Wi-Fi successful connection ![](https://img.shields.io/badge/status-todo-red)

## Features
Below a list of Gateway main features:
- Listening for new json string from External module via LoRa communication channel
- Hardware watchdog to auto recover from all kind of unexpected failures
- Discard LoRa incoming packets with CRC errors
- Parse LoRa incoming packets to extract weather values from json string 
- Perform a semantic analysis on weather data in json strings ![](https://img.shields.io/badge/status-todo-red)
- Add timestamp to json string (timestamp received by NTP Server)
- Routing json strings received by LoRa connection on TCP/IP connection using MQTT protocol

## Hardware
The Gateway is composed by following hardware component:

|Quantity|Name|Description|Alternative|
|--|--|--|--|
|1|[Lilygo SX1278 LoRa ESP32 443Mhz](http://www.lilygo.cn/prod_view.aspx?TypeId=50003&Id=1133&FId=t3:50003:3)|Microcontroller SX1278 ESP32 with LoRa trasmitter at 433Mhz|Equivalent ESP32 with LoRa trasmitter at 433Mhz (for example  [Heltec WiFi LoRa 32](https://heltec.org/project/wifi-lora-32/))|

## Software
The Gateway source code is uploaded in [firmware-gateway](https://github.com/pasgabriele/lora-weather-station/tree/main/Gateway/firmware-gateway) folder.
The code has been written using [Visul Studio Code](https://code.visualstudio.com/) and the [PlatformIO plugin](https://platformio.org/), therefore you can clone this repository directly on the above plaftorm.

### Description

All incoming LoRa packets are processed by the Gateway. It's listening for LoRa packets and when a new one comes, the first check is for the transmission integrity using CRC LoRa check function. If the CRC is wrong the packet is discarded. If the CRC is ok, the incoming packet is accepted and parsed to extract all weather values from json string. Then, a timestamp (requested to NTP Server) is added to json string and this complete json string is published to MQTT Broker on topic `weather` for the WeeWX server. Moreover, to avoid Gateway freeze, a configurable watchdog timer (default is 8 minutes) is inserted in the source code to perform an auto recover in case of unexpected failures.

The main functions of Gateway are:
- lora_connection(): to connect the Gateway to LoRa network and enable the CRC check for incoming LoRa packages
- wifi_connection(): to connect the Gateway to WiFi network
- parseJson(int packetSize): to receive the incoming LoRa packets and to extract all weather values from json string. During the extraction, the Gateway gets from NTP server the current timestamp and adds it to json string.
- sendToMQTTBroker(): to connect the Gateway to MQTT Broker and publish the json string on `weather` topic. After the string has been published, the Gateway disconnect the link to MQTT Broker.

