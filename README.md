# lora-weather-station

A complete weather station composed of three separate modules. 
- **External module**, which aims to collect all weather data from the sensors and to trasmit them to the Gateway.
- **Gateway**, which aims to receive the weather data from External module (via LoRa network) and to route them on TCP/IP network.
- **WeeWX server**, which aims to store and to permit the web access to weather data.

In the following image an overview of its architecture and components.

![weather station architecture](https://github.com/pasgabriele/lora-weather-station/blob/main/weather-station-architecture.svg)
 
This repository is divided into components as defined above:
- **WeeWX Server folder:** contains all necessary files to install and to configure the WeeWX server.
- **Gateway folder:** contains all necessary files to install and to configure the Gateway module.
- **External module folder:** contains all necessary files to install and to configure the External module.

For each folder there is a Readme.md file with all details for the specific module. This Readme.md file is just a overview on all weather station system.

## Features
The weather station main features are:
- Self-powered External module (via batteries and solar panel charger circuit)
- Wireless External module
- Access to weather data via MQTT
- Information on temperature, humidity, pressure, wind speed and direction, UV index, rain accumulation and particulate matter
- Access to real time and history weather data via Web GUI
- Graphical weather data rappresentation
