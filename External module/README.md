
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
  - Wind direction ![](https://img.shields.io/badge/status-ok-green)
  - UV index ![](https://img.shields.io/badge/status-ok-green)
  - Rain accumulation ![](https://img.shields.io/badge/status-todo-red)
- Weather data reading every 2,8 seconds
- Weather data encapsulation in json string
- Long distance and low energy LoRa communication with the Gateway
- Batteries powered
- Batteries voltage monitoring
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
|2|[RJ11 6P6C female PCB socket](https://www.aliexpress.com/item/1005001419331726.html)|RJ11 6P6C female socket for PCB||
|1|[Phoenix 2P connector](https://www.aliexpress.com/item/32819689207.html)|Phoenix 2P connector 5mm pitch for PCB||
|1|[Switch 2 position](https://www.aliexpress.com/item/32799198160.html)|Toggle switch 2 position 2.54mm pitch||
|1|[PCB 2 batteries case](https://www.aliexpress.com/item/4001009601436.html)|2 batteries case for PCB||
|3|[10k resistor](https://www.aliexpress.com/item/1005001649069962.html)|10k ohm resistor||
|2|[27k resistor](https://www.aliexpress.com/item/1005001649069962.html)|27k ohm resistor||47k ohm resistor
|7|[XH2.54 4P connector](https://www.aliexpress.com/item/32959016223.html)|XH2.54mm 4P connector ||
||[Male and female 2.54mm breakable pin header](https://www.aliexpress.com/item/32724478308.html)|Single row male and female 2.54mm breakable pin header PCB JST connector strip||
|1|[External module PCB](https://github.com/pasgabriele/lora-weather-station/tree/main/External%20module/pcb-external-module)|PCB for the external module||
|1|[Sensors shield](https://www.aliexpress.com/item/32969306380.html)|Shield for temperature, humidity and preassure sensor||
|1|Junction box|Outdoor PVC waterproof electrical junction boxe to store the assemblated PCB, UV index sensor and solar panel||

### Wiring schema and PCB
In the following the wiring schema for External module:

![external module schema](https://github.com/pasgabriele/lora-weather-station/blob/main/External%20module/Schematic_external-module-wiring_2021-06-20.svg)

and the PCB created to merge all External module components:

![external module pcb](https://github.com/pasgabriele/lora-weather-station/blob/main/External%20module/pcb-external-module/PCB_PCB_2020-12-13_09-27-49_stampato_2021-06-20.svg)

**Note 1:** The Lilygo SX1278 LoRa ESP32 is powered by the TP4056 output via SH1.25 battery interface.

**Note 2:** All weather sensors are powered by the Lilygo SX1278 LoRa ESP32 3V3 and GND pins.

**Note 2:** Below 3,65V battery voltage, the analog pin values (used for wind direction and battery measurement) are no long reliable.

## Software
The External module source code is uploaded in [firmware-external-module](https://github.com/pasgabriele/lora-weather-station/tree/main/External%20module/firmware-external-module) folder.
The code has been written using [Visul Studio Code](https://code.visualstudio.com/) and the [PlatformIO plugin](https://platformio.org/), therefore you can clone this repository directly on the above plaftorm.

### Description

All weather data read from sensors are processed by the External module, inserted in json string (for example `{"id":3908,"supplyVoltage":3.747275,"outTemp":26.61,"outHumidity":49.11914,"pressure":935.8403,"windSpeed":0,"windDir":157.5,"UV":0}`) and send, via LoRa wireless communication (433Mhz), to the Gateway.

The External module is fully self powered via 2 li-ion 18650 batteries and a solar panel charger. The External module has been projected to work as following:

- reads sensors values
- composes the json string
- sends the json string to Gateway

and repeat the cicle approximately every 2.8 seconds.

No Deep Sleep mode has been enable. In this way the weather data is captured in real time. This is important specially for wind speed and direction and rain accumulation.

### Temperature, humidity and pressure measurement
The temperature, humidity and pressure measurement is provided by BME280 sensor. In the setup function the BME280 sensor is initilizated using the function BMEInitialization(). If the initialization is ok then, in the loop, the BMEReading() reads the temperature, humidity and pressure data and inserts them in the BMETemperature, BMEHumidity and BMEPressure variables. These will used to compose the json string.

### UV index measurement
The UV index measurement is provided by VEML6075 sensor. In the setup function the VEML6075 sensor is initilizated using the function UVInitialization(). If the initialization is ok then, in the loop, the UVReading() reads the UV index data and inserts it in the UVIndex variables. This will used to compose the json string.

### Wind speed measurement
The wind speed measurement is derived by: (http://cactus.io/hookups/weather/anemometer/davis/hookup-arduino-to-davis-anemometer-wind-speed).

It works as following:

As describe in Spurkfun Weather Meter Kit datasheet, a wind speed of 2.401km/h causes the switch to close once per second, then the wind speed measurement can be executed counting the numbers of switch closed in a sample time. Therefore, when the External module executes the windSpeedReading() function, it actives the pulses measurement (activating the interrupt) for 2,401 seconds (sample window for wind measurement), then stops the pulses measurement (disabling the interrupt) and calculates the wind speed in this 2,401 seconds window.

### Wind direction measurement
The wind direction measurement is provided by windDirectionReading() function. It reads the analog value from the PIN connected to the Spurkfun Weather Meter Kit Wind Vane component (using the 10k ohm resistor) and converts this raw value wind direction degree. As describe in the datasheet, a specified voltage value maps a specific wind direction. Therefore the windDirectionReading() function maps the analog raw value to wind direction and return this in degrees value. The analog value is a AVG on 50 consecutive reads.

### Rain measurement ![](https://img.shields.io/badge/status-todo-red)
The rain measurement is provided by rainReading function. As describe in Spurkfun Weather Meter Kit datasheet, every 0.2794mm of rain causes the switch to close once, then the rain measurement can be executed counting the numbers of switch closed. Due to the External Module go to sleep for a defined time, is necessary to count the rain switch close during the normal mode and during the sleep mode too. To do this, there are 2 different counters:
 - rainCounterDuringSleep
 - rainCounterDuringActive 

It works as following:

During the normal mode, at startup time, a interrupt function to monitor the rain switch close is enabled. If a rain switch close is detected, the interrupt function increments the rainCounterDuringActive counter, then, when the rainReading function is called, it uses the counter to calculate the rain amount.

Instead, during the sleep mode, the External module monitors the rain GPIO and if it detects a rain switch close, wake-up the External module, increases the rainCounterDuringSleep counter and executes the normal mode above described.

### Battery voltage measurement
The battery voltage measurement is provided by batteryLevel() function. It reads the analog value from the PIN connected to battery and converts this raw value in voltage measurement. The analog value is a AVG on 50 consecutive reads.

The function used for voltage measurament is the following: 

*battery voltage = c * analog value*

voltage = c * analog value

where c is the constant 0,00172131 calculate in the following way:

Using R1 and R2 resistors the maximun input voltage for GPIO33 when the battery is totally full is:

Volt on GPIO32 = (Max battery volt * R2)/(R1+R2)

then 

Volt on GPIO32 = (4,2V * 27k)/(27k+27k) = 113400/54000 = 2,1V

to determinate the c constant, I read the analog value from GPIO32 when the battery voltage is 4,2. This analog value was 2440, then I calculated the c contanst as following:

c = 4,2V/2440 = 0,00172131147541

### Json string creation and LoRa sending
When all weather data have been read, these are inserted in a json string using the composeJson function, then the string is sent to the Gateway using LoRaSend function via LoRa connection. After sending the string, the External module restart the cicle.
