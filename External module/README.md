
# External module

## TODO ![](https://img.shields.io/badge/status-todo-red)

- Rainfall function

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

### Wiring schema
In the following the wiring schema for External module:

![external module schema](https://github.com/pasgabriele/lora-weather-station/blob/main/External%20module/Schematic_external-module-wiring_2021-06-20.svg)

In the following paragraph are detailed and explained all single part of External module.

#### Power system
The External Module is powered by 2 18650 3.7V 3400mAh Li-Ion batteries in parallel (6400mAh) recharged by a 6V 750ma solar panel via the TP4056 module. The TP4056 module accepts between 4 and 8 input voltage, therefore the solar panel can be directly connected to it via IN+ and IN- pins. The TP4056 recharges the 2 batteries with a maximum of 1000mA via the B+ and B+ pins. At the same time, the batteries, via the OUT+ and OUT- pins of TP4056, power the Lilygo ESP32 microcontroller using the dedicated onboard SH1.25-2 battery interface. Using this dedicated interface no others external components are necessary to voltage regulation. A 2 position switch is inserted between the batteries and the microcontroller so that all sensors and the microcontroller can be turned off. Note that the batteries recharge circuit can never be turned off because it's before the switch.

All weather sensors are powered directly by microcontroller 3V3 and GND pins without using external voltage regulator due to all sensors required 3,3V to operate.

#### External module lifetime without recharge system enabled
Measuring the energy consumption of the External module using an amperometer, it has been registered that it requires about 20mA during the weather reading and peaks of 130mA during the LoRa packets trasmissions.
Using this values, we can calculate the External module lifetime witout charging system enabled:

|Description|Measurement|
|--|--|
|Weather data reading|2500ms|
|LoRa trasmission|230ms|
|Weather data reading + LoRa trasmission (cicle)|about 2,8s|
|Number of cicles per hour|3600/2,8s = 1286 cicles|
|Current consumption per hour for weather data reading|1286*20mA*2,5/3600 = 17,87mA|
|Current consumption per hour for LoRa trasmission|1286*130mA*0,23/3600 = 18,90mA|
|Battery lifetime in hours|6800/(17,87+18,90) = 6800/36,77 = 184,92 hours|
|Battery lifetime in days|184,92/24 = 7,7 days|

These calculations are theoretical only. There are other factors that influence the results (real battery capacity, weather conditions, etc.). Moreover, it's very important declare that when the batteries drop below 3.65V, the External module works badly and therefore the weather data readings are no longer reliable.
Based on these considerations it can be said that the External module, in the absence of a recharging system, is able to live for 3/4 days.

#### Battery voltage monitoring system ![](https://img.shields.io/badge/status-todo-red)

Using an analog pin (GPIO33) of microcontroller, it possibles to check the voltage of the battery, but all ADC pin exepct voltages between 0 and 3,3 volts instead the battery output voltage is 4,2 volts when it is totally charged. To solve this problem a voltage divider has been connected to the battery to divide the voltage by 2 and to have an maximun voltage of 2,1 volts when battery is totally charged. To do this, 2 27k ohm resistors have been inserted as reported in the following circuit:

![battery monitor](https://raw.githubusercontent.com/pasgabriele/lora-weather-station/main/External%20module/Schematic_battery%20monitor_2021-06-22.svg)

Using this voltage divider we have: 

<img src="https://render.githubusercontent.com/render/math?math=VoltOnGPIO33=(maxBatteryVoltage*R2)/(R1%2BR2)=(4,2V*27k)/(54k)=2,1V">

With this, we can measure the voltage applied to GPIO33 and then calculate the battery level. As already mentioned the voltages on GPIO33 shifts between 0 and 3,3 volts then between 0 and 4095 values (the ADC pin has 12bit resolution), so we can establish a constant to calculate the voltage applied to the pin based on its value. This constant, theoretically, will be:

<img src="https://render.githubusercontent.com/render/math?math=c=3300/4095=0,8058608059">

As we are applying a voltage divider and the voltage applied to the pin is half the voltage of the battery, our constant should be:

<img src="https://render.githubusercontent.com/render/math?math=c=0,8058608059*2=1,6117216118">

This means, for each unit in ADC pin we have 1,6117216118 mVolts applied to it.

For example, if the read value on ADC pin is 2320, then the voltage applied to the pin should be:

<img src="https://render.githubusercontent.com/render/math?math=V_batt=2320*1,6117216118=3739,194139376=3,74V">


ADC pins are not that precise, so the value of our constant should be adjusted to a level we consider it is valid for our components. In my case, after doing some testings I have concluded that the best value for the conversion factor is 1.7.








An important aspect of the External module is the tracking of battery voltage level. To do this, the following circuit has been provided:

![battery monitor](https://raw.githubusercontent.com/pasgabriele/lora-weather-station/main/External%20module/Schematic_battery%20monitor_2021-06-22.svg)

The voltage output from the battery shift from 4,2V (when it is full) and 2,4 (when it is completely discharged). 

see: https://www.pangodream.es/esp32-getting-battery-charging-level and https://www.settorezero.com/wordpress/arduino-nano-33-iot-wifi-ble-e-imu/

#### i2c communication ![](https://img.shields.io/badge/status-todo-red)

#### Anemometer ![](https://img.shields.io/badge/status-todo-red)

#### Wind vane ![](https://img.shields.io/badge/status-todo-red)

#### Rain gauge ![](https://img.shields.io/badge/status-todo-red)

### PCB

Using the above schema and explanations, the following PCB has been created to merge all External module components:

![external module pcb](https://github.com/pasgabriele/lora-weather-station/blob/main/External%20module/pcb-external-module/PCB_PCB_2020-12-13_09-27-49_stampato_2021-06-20.svg)

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

As describe in Spurkfun Weather Meter Kit datasheet, a wind speed of 2.401km/h causes the switch to close once per second, then the wind speed measurement can be executed counting the numbers of switch closed in a sample time. Therefore, when the External module executes the windSpeedReading() function, it actives the pulses measurement (activating the interrupt) for 2,401 seconds (sample window for wind measurement), then stops the pulses measurement (disabling the interrupt), calculates the wind speed in this 2,401 seconds window and stores this value in the windSpeed variable. This will used to compose the json string.

### Wind direction measurement
The wind direction measurement is provided by windDirectionReading() function. It reads the analog value from the PIN connected to the Spurkfun Weather Meter Kit Wind Vane component (using the 10k ohm resistor) and converts this raw value in wind direction degree. As describe in the datasheet, a specified voltage value maps a specific wind direction. Therefore the windDirectionReading() function maps the analog raw value to wind direction and return this in degrees value. The analog value is a AVG on 50 consecutive reads. This degree value is stored in the windDir variable and it will used to compose the json string. 

### Rain measurement ![](https://img.shields.io/badge/status-todo-red)
~~The rain measurement is provided by rainReading function. As describe in Spurkfun Weather Meter Kit datasheet, every 0.2794mm of rain causes the switch to close once, then the rain measurement can be executed counting the numbers of switch closed. Due to the External Module go to sleep for a defined time, is necessary to count the rain switch close during the normal mode and during the sleep mode too. To do this, there are 2 different counters:~~
~~- rainCounterDuringSleep~~
~~- rainCounterDuringActive~~

~~It works as following:~~

~~During the normal mode, at startup time, a interrupt function to monitor the rain switch close is enabled. If a rain switch close is detected, the interrupt function increments the rainCounterDuringActive counter, then, when the rainReading function is called, it uses the counter to calculate the rain amount.~~

~~Instead, during the sleep mode, the External module monitors the rain GPIO and if it detects a rain switch close, wake-up the External module, increases the rainCounterDuringSleep counter and executes the normal mode above described.~~

### Battery voltage measurement
The battery voltage measurement is provided by batteryLevel() function. It reads the analog value from the PIN connected to battery and converts this raw value in voltage measurement. The analog value is a AVG on 50 consecutive reads.

The function used for voltage measurament is the following: 

<img src="https://render.githubusercontent.com/render/math?math=batteryVoltage=c*analogValue">

where *c* is the constant *0,00173* calculated in the following way:

Using R1 and R2 resistors, when the battery is totally full, the maximun input voltage for GPIO33 is:

<img src="https://render.githubusercontent.com/render/math?math=maxVoltOnGPIO32=(maxBatteryVoltage*R2)/(R1%2BR2)">

then 

<img src="https://render.githubusercontent.com/render/math?math=maxVoltOnGPIO32=(4,2*27000)/(27000%2B27000)=113400/54000=2,1V">

to determinate the *c* constant, I read the analog value from GPIO32 when the battery voltage is 4,2. This analog value was 2427, then I calculated the *c* contanst as following:

<img src="https://render.githubusercontent.com/render/math?math=c=4,2/2427=0,00173">

The voltage measurement is stored in the volt variable and it will used to compose the json string.

### Json string creation and LoRa sending
When all weather data have been read, these are inserted in a json string using the composeJson() function, then the string is sent to the Gateway using LoRaSend(String packet) function via LoRa connection. After sending the string, the External module restart the cicle.
