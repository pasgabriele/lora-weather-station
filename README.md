# lora-weather-station
A complete wireless auto powered online weather station. In the following imange an overview of its architecture and components.

![weather station architecture](https://raw.githubusercontent.com/pasgabriele/lora-weather-station/main/weather-station-architecture.svg)
 
The repository is divided into components:
- **Server folder:** contains all necessary files to install and to configure the RPI Nano which aims to store and to permit the web access to weather data.
- **Gateway folder:** contains all necessary files to install and to configure the Gateway module which aims to receive the weather data from External module (via LoRa network) and to route them on TCP/IP network.
- **External module folder:** contains all necessary files to install and to configure the External module which aims to collect all weather data from the sensors and to trasmit them to the Gateway. 
