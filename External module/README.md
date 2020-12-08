
# External module

## TODO ![](https://img.shields.io/badge/status-todo-red)

 - Verify battery voltage measurement
 - Verify ESP32 powered from TP4056 output to SH1.25 input
 - Verify sensors powered from ESP32 3V3 pin
 - Modify rain and wind wiring. GPIO35 and GPIO34 cannot be used. They are pin input only!!! Use the following GPIO:
   - GPIO13 (digital): for windspeed (tested)
   - GPIO32 (analog): for windir (tested)
   - GPIO12 (digital): for rain (to be tested)

 Test GPIO13 (analog) for Wind Direction GPIO32 (analog) for Rain

The External module is the set of all necessary sensors to detect weather data. It is able to detect the following measures:
- Temperature ![](https://img.shields.io/badge/status-ok-green)
- Humidity ![](https://img.shields.io/badge/status-ok-green)
- Pressure ![](https://img.shields.io/badge/status-ok-green)
- Wind speed ![](https://img.shields.io/badge/status-ok-green)
- Wind direction ![](https://img.shields.io/badge/status-todo-red)
- UV index ![](https://img.shields.io/badge/status-todo-red)
- Rain accumulation ![](https://img.shields.io/badge/status-todo-red)
- Drew point ![](https://img.shields.io/badge/status-todo-red)
- Wind chill ![](https://img.shields.io/badge/status-todo-red)
- Heat index ![](https://img.shields.io/badge/status-todo-red)

All weather data coming from sensors are processed by the External module, inserted in json string (for example `{"temp":27.23, "humi":62.05, "pres":1013.25}`) and sended, via LoRa wireless communication (433Mhz), to the Gateway.

The External module is fully self powered via 2 li-ion 18650 batteries and a solar panel charger. To optimize the batteries life, the External module has been projected to work as following:

- reads sensors values
- compones the json string
- sends the json string to Gateway

then goes in Deep Sleep mode for a configurable time (default is 15 minutes). Of couse, the batteries saving necessity affects the weather data update frequency. With 15 minutes of Deep Sleep mode you do not have a real time weather situation. This is insignificant for some weather data (for example temperature, humdity and preassure) because this measures don't have great variations in 15 minutes, but it could be significant for other weather data (for example wind speed and direction). For this reason you can set the Deep Sleep time based on your necessities.

### Wind speed measurement
The wind speed measurement is ereditated by [cactus.io web site](http://cactus.io/hookups/weather/anemometer/davis/hookup-arduino-to-davis-anemometer-wind-speed). It works as following:

As described above, the External module starts, reads the sensors values, compones the json string, sends it to the Gateway and goes to Deep Sleep. For the specific wind speed measurement, when the External module executes the readWind function, it actives the pulse measurement (via interrupt) for 3 seconds (via delay), then stop the pulse measurement (disabing the interrupt) and calculates the windspeed AVG in the 3 seconds.

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

![external module schema](https://github.com/pasgabriele/lora-weather-station/blob/main/External%20module/external-module.svg)

and the PCB created to merge all External module components:

![external module pcb](https://github.com/pasgabriele/lora-weather-station/blob/main/External%20module/pcb-external-module/pcb-external-module-v1.svg)

**Note 1:** ![](https://img.shields.io/badge/status-toverify-yellow) The Lilygo SX1278 LoRa ESP32 is powered by the TP4056 output via SH1.25 battery interface.

**Note 2:** ![](https://img.shields.io/badge/status-toverify-yellow) All weather sensors are powered by the Lilygo SX1278 LoRa ESP32 3V3 and GND pins.

## Software
The External module source code is uploaded in [firmware-external-module](https://github.com/pasgabriele/lora-weather-station/tree/main/External%20module/firmware-external-module) folder.
The code has been written using [Atom IDE](https://atom.io/) and the [PlatformIO plugin](https://platformio.org/), therefore you can clone this repository directly on the above plaftorm.

### Requirements
To compile correctly the source code is required to install the following board and libraries from PlatformIO interface in Atom:

 #### Board
 - esp32doit-devkit-v1

 #### Libraries
 - sandeepmistry/LoRa@^0.7.2
 - adafruit/Adafruit BME280 Library@^2.1.0
 - bblanchon/ArduinoJson@^6.16.1
 - adafruit/Adafruit Unified Sensor@^1.1.4
