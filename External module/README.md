
# External module

## TODO ![](https://img.shields.io/badge/status-todo-red)

- Verify battery voltage measurement
- Verify ESP32 powered from TP4056 output to SH1.25 input
- Verify sensors powered from ESP32 3V3 pin

## Features
Below a list of External module main features:
- It is able to detect the following measures:
  - Temperature ![](https://img.shields.io/badge/status-ok-green)
  - Humidity ![](https://img.shields.io/badge/status-ok-green)
  - Pressure ![](https://img.shields.io/badge/status-ok-green)
  - Wind speed ![](https://img.shields.io/badge/status-ok-green)
  - Wind gust ![](https://img.shields.io/badge/status-ok-green)
  - Wind direction ![](https://img.shields.io/badge/status-ok-green)
  - Time ![](https://img.shields.io/badge/status-todo-red)
  - UV index ![](https://img.shields.io/badge/status-ok-green)
  - Rain accumulation ![](https://img.shields.io/badge/status-ok-green)
- Weather data reading every 5 minutes
- Weather data encapsulation in json string
- Long distance and low energy LoRa communication with the Gateway
- Deep sleep mode
- Batteries powered
- Batteries voltage monitoring ![](https://img.shields.io/badge/status-toverify-yellow)
- Solar charging

## Hardware
The External module is composed by following hardware components:

|Quantity|Name|Description|Alternative|
|--|--|--|--|
|1|[Lilygo SX1278 LoRa ESP32 443Mhz](http://www.lilygo.cn/prod_view.aspx?TypeId=50003&Id=1133&FId=t3:50003:3)|Microcontroller SX1278 ESP32 with LoRa trasmitter at 433Mhz|Equivalent ESP32 with LoRa trasmitter at 433Mhz (for example  [Heltec WiFi LoRa 32](https://heltec.org/project/wifi-lora-32/))|
|1|[TP4056](https://it.aliexpress.com/item/32986135934.html)|Battery charger 5V 1A||
|1|[Solar panel](https://it.aliexpress.com/item/32877897718.html)|6V 750mA solar panel||
|2|[Panasonic 18650](https://it.aliexpress.com/item/4000484192899.html)|Batteries Panasonic 18650 NCR18650B 3.7V 3400mAh Li-Ion with PCB||
|1|[BME280](https://it.aliexpress.com/item/32849462236.html)|Temperature, humidity and preassure sensor||
|1|[Spurkfun Weather Meter Kit](https://www.sparkfun.com/products/15901)|Wind speed, wind direction and rain accumulation sensor||
|1|[VEML6075](https://it.aliexpress.com/item/32843641073.html)|UV index sensor||
|1|[DS3231](https://it.aliexpress.com/item/32925920564.html)|Real time clock||
|2|[RJ11 6P6C female PCB socket](https://www.aliexpress.com/item/1005001419331726.html)|RJ11 6P6C female socket for PCB||
|1|[Phoenix 2P connector](https://www.aliexpress.com/item/32819689207.html)|Phoenix 2P connector 5mm pitch for PCB||
|1|[Switch 2 position](https://www.aliexpress.com/item/32799198160.html)|Toggle switch 2 position 2.54mm pitch||
|1|[PCB 2 batteries case](https://www.aliexpress.com/item/4001009601436.html)|2 batteries case for PCB||
|3|[10k resistor](https://www.aliexpress.com/item/4000695402017.html)|10k ohm resistor||
|1|[100k resistor](https://www.aliexpress.com/item/4000695402017.html)|100k ohm resistor||
|1|[27k resistor](https://www.aliexpress.com/item/4000695402017.html)|27k ohm resistor||
|6|[XH2.54 4P connector](https://www.aliexpress.com/item/32959016223.html)|XH2.54mm 4P connector ||
||[Male and female 2.54mm breakable pin header](https://www.aliexpress.com/item/32724478308.html)|Single row male and female 2.54mm breakable pin header PCB JST connector strip||
|1|[External module PCB](https://github.com/pasgabriele/lora-weather-station/tree/main/External%20module/pcb-external-module)|PCB for the external module||
|1|[Sensors shield](https://www.aliexpress.com/item/32969306380.html)|Shield for temperature, humidity and preassure sensor||
|1|Junction box|Outdoor PVC waterproof electrical junction boxe to store the assemblated PCB, UV index sensor, real time clock and solar panel||

### Wiring schema and PCB
In the following the wiring schema for External module:

![external module schema](https://github.com/pasgabriele/lora-weather-station/blob/main/External%20module/schematic-external-module.svg)

and the PCB created to merge all External module components:

![external module pcb](https://github.com/pasgabriele/lora-weather-station/blob/main/External%20module/pcb-external-module/pcb-external-module.svg)

**Note 1:** ![](https://img.shields.io/badge/status-toverify-yellow) The Lilygo SX1278 LoRa ESP32 is powered by the TP4056 output via SH1.25 battery interface.

**Note 2:** ![](https://img.shields.io/badge/status-toverify-yellow) All weather sensors are powered by the Lilygo SX1278 LoRa ESP32 3V3 and GND pins.

## Software
The External module source code is uploaded in [firmware-external-module](https://github.com/pasgabriele/lora-weather-station/tree/main/External%20module/firmware-external-module) folder.
The code has been written using [Atom IDE](https://atom.io/) and the [PlatformIO plugin](https://platformio.org/), therefore you can clone this repository directly on the above plaftorm.

### Description

All weather data read from sensors are processed by the External module, inserted in json string (for example `{outTemp:27.23,outHumidity:62.05,pressure:1013.25}`) and send, via LoRa wireless communication (433Mhz), to the Gateway.

The External module is fully self powered via 2 li-ion 18650 batteries and a solar panel charger. To optimize the batteries life, the External module has been projected to work as following:

- reads sensors values
- composes the json string
- sends the json string to Gateway

then goes in Deep Sleep mode for a configurable time (default is 5 minutes). Of course, the batteries saving necessity affects the weather data update frequency. With 5 minutes of Deep Sleep mode you do not have a real time weather situation. This is insignificant for some weather data (for example temperature, humidity and pressure) because this measures don't have great variations in 5 minutes, but it could be significant for other weather data (for example wind speed and direction). For this reason you can set the Deep Sleep time based on your necessities.

### Temperature, humidity and pressure measurement
The temperature, humidity and pressure measurement is provided by BME280 sensor. The function used in the firmware is BMEReading: this function tries to begin the BME sensor and if done it reads the data and inserts them in the BMETemperature, BMEHumidity and BMEPressure variables. BMEReading function is called every wakeup.

### UV index measurement
The UV index measurement is provided by VEML6075 sensor. The function used in the firmware is UVReading: this function tries to begin the VEML6075 sensor and if done it reads the data and inserts them in the UVIndex variable. UVReading function is called every wakeup.

### Wind speed measurement
The wind speed measurement is derived by: (http://cactus.io/hookups/weather/anemometer/davis/hookup-arduino-to-davis-anemometer-wind-speed), (https://github.com/rpurser47/weatherstation) and (https://github.com/switchdoclabs/SDL_Weather_80422).

It works as following:

As describe in Spurkfun Weather Meter Kit datasheet, a wind speed of 2.401km/h causes the switch to close once per second, then the wind speed measurement can be executed counting the numbers of switch closed in a sample time. Therefore, when the External module executes the windReading function, it actives the pulses measurement (activating the interrupt) for 5 seconds (sample window for wind measurement), then stops the pulses measurement (disabling the interrupt) and calculates the wind speed and gust in this 5 seconds window. windReading function is called every wakeup.

### Wind direction measurement
The wind direction measurement is provided by windDirectionReading function inserted in windReading function. It reads the analog value from the PIN connected to the Spurkfun Weather Meter Kit Wind Vane component (using the 10k ohm resistor) and converts this raw value in voltage measurement. As describe in the datasheet, a specified voltage value maps a specific wind direction. Therefore the windDirectionReading function maps the voltage to wind direction and return this in degrees value. The analog value is a AVG on 50 consecutive reads. windDirectionReading function is called every wakeup.

### Rain measurement
The rain measurement is provided by rainReading function. As describe in Spurkfun Weather Meter Kit datasheet, every 0.2794mm of rain causes the switch to close once, then the rain measurement can be executed counting the numbers of switch closed. Due to the External Module go to sleep for a defined time, is necessary to count the rain switch close during the normal mode and during the sleep mode too. To do this, there are 2 different counters:
 - rainCounterDuringSleep
 - rainCounterDuringActive 

It works as following:

During the normal mode, at startup time, a interrupt function to monitor the rain switch close is enabled. If a rain switch close is detected, the interrupt function increments the rainCounterDuringActive counter, then, when the rainReading function is called, it uses the counter to calculate the rain amount.

Instead, during the sleep mode, the External module monitors the rain GPIO and if it detects a rain switch close, wake-up the External module, increases the rainCounterDuringSleep counter and executes the normal mode above described.

### Battery voltage measurement
The battery voltage measurement is provided by batteryLevel function. It reads the analog value from the PIN connected to battery and converts this raw value in voltage measurement. The analog value is a AVG on 50 consecutive reads. batteryLevel function is called every wakeup.

### Json string creation and LoRa sending
When all weather data have been read, these are inserted in a json string using the composeJson function, then the string is sent to the Gateway using LoRaSend function via LoRa connection. After sending the string, the External module go in Deep Sleep for the configured time.

### Requirements
To compile correctly the source code is required to install the following board and libraries from PlatformIO interface in Atom:

 #### Board
 - esp32doit-devkit-v1

 #### Libraries
 - sandeepmistry/LoRa@^0.7.2
 - adafruit/Adafruit BME280 Library@^2.1.0
 - bblanchon/ArduinoJson@^6.16.1
 - adafruit/Adafruit Unified Sensor@^1.1.4
