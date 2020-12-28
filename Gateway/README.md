# Gateway

## TODO ![](https://img.shields.io/badge/status-todo-red)

- Verify Gateway watchdog (insert the WDT in parse function) ![](https://img.shields.io/badge/status-todo-red)
- Weather data received semantic analysis ![](https://img.shields.io/badge/status-todo-red)

## Features
Below a list of Gateway module main features:
- Listening for new json string from External module via LoRa communication channel
- Hardware watchdog to auto recover from all kind of unexpected failures
- Discard LoRa incoming packets with CRC errors
- Parse LoRa incoming packets to extract weather values from json string 
- Perform a semantic analysis on weather data in json strings ![](https://img.shields.io/badge/status-todo-red)
- Add timestamp to json string (timestamp received by NTP Server)
- Routing json strings received by LoRa connection on TCP/IP connection using MQTT protocol

## Hardware
The Gateway module is composed by following hardware component:

|Quantity|Name|Description|Alternative|
|--|--|--|--|
|1|[Lilygo SX1278 LoRa ESP32 443Mhz](http://www.lilygo.cn/prod_view.aspx?TypeId=50003&Id=1133&FId=t3:50003:3)|Microcontroller SX1278 ESP32 with LoRa trasmitter at 433Mhz|Equivalent ESP32 with LoRa trasmitter at 433Mhz (for example  [Heltec WiFi LoRa 32](https://heltec.org/project/wifi-lora-32/))|

## Software
The External module source code is uploaded in [firmware-external-module](https://github.com/pasgabriele/lora-weather-station/tree/main/External%20module/firmware-external-module) folder.
The code has been written using [Visul Studio Code](https://code.visualstudio.com/) and the [PlatformIO plugin](https://platformio.org/), therefore you can clone this repository directly on the above plaftorm.

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
