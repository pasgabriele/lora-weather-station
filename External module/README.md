
# External module

## TODO ![](https://img.shields.io/badge/status-todo-red)

- Try to change LoRa signalBandwidth and txPower to reduce trasmission power consumption (https://iopscience.iop.org/article/10.1088/1742-6596/1407/1/012092/pdf)
- Replace TP4056 with Adafruit Solar Charger (https://www.adafruit.com/product/4755)
- Add an INA219 for solar panel voltage and current measurement
- Add an INA219 for battery voltage and current measurement

## Features
Below a list of External module main features:
- It is able to detect the following measures:
  - Temperature ![](https://img.shields.io/badge/status-ok-green)
  - Humidity ![](https://img.shields.io/badge/status-ok-green)
  - Pressure ![](https://img.shields.io/badge/status-ok-green)
  - Wind speed ![](https://img.shields.io/badge/status-ok-green)
  - Wind direction ![](https://img.shields.io/badge/status-ok-green)
  - UV index ![](https://img.shields.io/badge/status-ok-green)
  - Rain accumulation ![](https://img.shields.io/badge/status-todo-green)
- Weather data reading every 2,8 seconds
- Weather data encapsulation in json string
- Long distance and low energy LoRa communication with the Gateway
- Batteries powered
- Batteries voltage and current monitoring
- Solar charged
- Solar panel voltage and current monitoring

## Components
The External module is composed by following hardware components:

|Quantity|Name|Description|Alternative|
|--|--|--|--|
|1|[Lilygo SX1278 LoRa ESP32 443Mhz](http://www.lilygo.cn/prod_view.aspx?TypeId=50003&Id=1133&FId=t3:50003:3)|Microcontroller SX1278 ESP32 with LoRa trasmitter at 433Mhz|Equivalent ESP32 with LoRa trasmitter at 433Mhz (for example  [Heltec WiFi LoRa 32](https://heltec.org/project/wifi-lora-32/))|
|1|[Adafruit Universal USB / DC / Solar Lithium Ion/Polymer charger - bq24074](https://www.adafruit.com/product/4755)|Solar Lithium Ion/Polymer charger||
|1|[Solar panel](https://it.aliexpress.com/item/32877897718.html)|6V 750mA solar panel||
|2|[Panasonic 18650](https://it.aliexpress.com/item/4000484192899.html)|Batteries Panasonic 18650 NCR18650B 3.7V 3400mAh Li-Ion with PCB||
|2|[INA219](https://it.aliexpress.com/item/1005001854701258.html)|Current and voltage monitor||
|1|[HT7333](https://it.aliexpress.com/item/32694851944.html)|3,3V Low Dropout (LDO) regulator||
|1|[BME280](https://it.aliexpress.com/item/32849462236.html)|Temperature, humidity and preassure sensor||
|1|[Spurkfun Weather Meter Kit](https://www.sparkfun.com/products/15901)|Wind speed, wind direction and rain accumulation sensor||
|1|[VEML6075](https://it.aliexpress.com/item/32843641073.html)|UV index sensor||
|7|[RJ11 6P4C female PCB socket](https://www.ebay.it/itm/294215189887)|RJ11 6P4C female socket for PCB||
|1|[Phoenix 2P connector](https://www.aliexpress.com/item/32819689207.html)|Phoenix 2P connector 5mm pitch for PCB||
|1|[Switch 2 position](https://www.aliexpress.com/item/32799198160.html)|Toggle switch 2 position 2.54mm pitch||
|1|[PCB 2 batteries case](https://www.aliexpress.com/item/4001009601436.html)|2 batteries case for PCB||
|3|[10k resistor](https://www.aliexpress.com/item/1005001649069962.html)|10k ohm resistor||
||[Male and female 2.54mm breakable pin header](https://www.aliexpress.com/item/32724478308.html)|Single row male and female 2.54mm breakable pin header PCB JST connector strip||
|1|[External module PCB](https://github.com/pasgabriele/lora-weather-station/tree/main/External%20module/pcb-external-module)|PCB for the external module||
|1|[Sensors shield](https://www.aliexpress.com/item/32969306380.html)|Shield for temperature, humidity and preassure sensor||
|1|Junction box|Outdoor PVC waterproof electrical junction boxe to store the assemblated PCB, UV index sensor and solar panel||

## Wiring schema
In the following the wiring schema for External module:

![external module schema](https://raw.githubusercontent.com/pasgabriele/lora-weather-station/main/External%20module/Schematic_external-module-wiring-v.2.2_2021-07-20.svg)

In the following paragraph are detailed and explained all single part of External module.

## Power system
The External Module power system core is the Adadruit Solar Charger (bq24074). It recharges the 2 18650 3.7V 3400mAh Li-Ion batteries (6400mAh in parallel) using the 6V 750ma solar panel and at the same time, it powers the Lilygo ESP32 microcontroller via the HT7333 LDO. The maximum batteries charge current is 1000ma. A 2 position switch is inserted between the batteries and the microcontroller so that all sensors and the microcontroller can be turned off. Note that the batteries charge circuit can never be turned off because it's before the switch.

All weather sensors are powered directly by microcontroller 3V3 and GND pins without using external voltage regulator due to all sensors required 3,3V to operate.

## External module lifetime without recharge system enabled
Measuring the energy consumption of the External module using an amperometer, it has been registered that it requires about 20mA during the weather reading (setting the microcontroller CPU frequency to 26Mhz) and peaks of 130mA during the LoRa packets trasmissions.
Using this values, we can calculate the External module lifetime witout charging system enabled:

|Description|Measurement|
|--|--|
|Weather data reading|2500ms|
|LoRa trasmission|230ms|
|Weather data reading + LoRa trasmission (cicle)|about 2,8s|
|Number of cicles per hour|3600 / 2,8s = 1286 cicles|
|Current consumption per hour for weather data reading|1286 * 20mA * 2,5 / 3600 = 17,87mA|
|Current consumption per hour for LoRa trasmission|1286 * 130mA * 0,23 / 3600 = 18,90mA|
|Battery lifetime in hours|6800 / (17,87 + 18,90) = 6800 / 36,77 = 184,92 hours|
|Battery lifetime in days|184,92 / 24 = 7,7 days|

These calculations are theoretical only. There are other factors that influence the results (real battery capacity, weather conditions, etc.). **Moreover, it's very important declare that when the batteries drop below 3.65V, the External module works badly and therefore the weather data readings are no longer reliable.**
Based on these considerations it can be said that the External module, in the absence of a recharging system, is able to live for 3/4 days.

## Battery voltage monitoring system

Using an analog pin (GPIO33) of microcontroller, it possibles to check the voltage of the battery, but all ADC pin exepct voltages between 0 and 3,3 volts instead the battery output voltage is 4,2 volts when it is totally charged. To solve this problem a voltage divider has been connected to the battery to divide the voltage by 2 and to have an maximun voltage of 2,1 volts when battery is totally charged. To do this, 2 27k ohm resistors have been inserted as reported in the following circuit:

![battery monitor](https://raw.githubusercontent.com/pasgabriele/lora-weather-station/main/External%20module/Schematic_battery%20monitor_2021-06-22.svg)

Using this voltage divider we have VoltOnGPIO33 = (maxBatteryVoltage * R2) / (R1 + R2) = (4,2V * 27k) / (54k) = 2,1V

With this, we can measure the voltage applied to GPIO33 and then calculate the battery level (see Battery voltage measurement section for software details).

## i2c communication
The i2c channel is used to permit communication between the microcontroller and the UV (VEML6075) and temperature, humidity and pressure sensors (BME280). This bus can be used for future purpose too, due to on PCB there are other 3 i2c sockets.

## Anemometer
The cup-type anemometer measures wind speed by closing a contact as a magnet moves past a switch. A wind speed of 2,401 km/h causes the switch to close once per second. The anemometer switch is connected to the inner two conductors of the RJ11 cable shared by the anemometer and wind vane (pin 2 and 3) and finally it is connected to the microcontroller via the shared wind vane RJ11. Below the detailed schema:

![anemometer wiring](https://raw.githubusercontent.com/pasgabriele/lora-weather-station/main/External%20module/Schematic_anemometer_2021-06-23.svg)

The Anemometer is connected to the microcontroller GPIO23 and GND. After that, all we need to do then is to monitor for button presses which is pretty straightforward. We can use the pin interrupts method to monitor the button press (tips). When the reed switch closes the circuit (pressing the button), it triggers a software event (see Wind speed measurement section for software details).

## Wind vane ![](https://img.shields.io/badge/status-todo-red)

## Rain gauge ![](https://img.shields.io/badge/status-todo-red)

## PCB

Using the above schema and explanations, the following PCB has been created to merge all External module components:

![external module pcb](https://raw.githubusercontent.com/pasgabriele/lora-weather-station/main/External%20module/pcb-external-module/PCB_PCB_2021-07-20.svg)

## Software description
The External module source code is uploaded in [firmware-external-module](https://github.com/pasgabriele/lora-weather-station/tree/main/External%20module/firmware-external-module) folder.
The code has been written using [Visul Studio Code](https://code.visualstudio.com/) and the [PlatformIO plugin](https://platformio.org/), therefore you can clone this repository directly on the above plaftorm.

All weather data read from sensors are processed by the External module, inserted in json string (for example `{"id":3908,"supplyVoltage":3.747275,"outTemp":26.61,"outHumidity":49.11914,"pressure":935.8403,"windSpeed":0,"windDir":157.5,"UV":0}`) and send, via LoRa wireless communication (433Mhz), to the Gateway.

The External module is fully self powered via 2 li-ion 18650 batteries and a solar panel charger. The External module has been projected to work as following:

- reads sensors values
- composes the json string
- sends the json string to Gateway

and repeat the cicle approximately every 2.8 seconds.

No Deep Sleep mode has been enable. In this way the weather data is captured in real time. This is important specially for wind speed and direction and rain accumulation.

## Temperature, humidity and pressure measurement
The temperature, humidity and pressure measurement is provided by BME280 sensor. In the setup function the BME280 sensor is initilizated using the function BMEInitialization(). If the initialization is ok then, in the loop, the BMEReading() reads the temperature, humidity and pressure data and inserts them in the BMETemperature, BMEHumidity and BMEPressure variables. These will used to compose the json string.

## UV index measurement
The UV index measurement is provided by VEML6075 sensor. In the setup function the VEML6075 sensor is initilizated using the function UVInitialization(). If the initialization is ok then, in the loop, the UVReading() reads the UV index data and inserts it in the UVIndex variables. This will used to compose the json string.

## Wind speed measurement
The wind speed measurement is derived by: (http://cactus.io/hookups/weather/anemometer/davis/hookup-arduino-to-davis-anemometer-wind-speed).

It works as following:

As describe in Spurkfun Weather Meter Kit datasheet, a wind speed of 2.401km/h causes the switch to close once per second, then the wind speed measurement can be executed counting the numbers of switch closed in a sample time. Therefore, when the External module executes the windSpeedReading() function, it actives the pulses measurement (activating the interrupt) for 2,401 seconds (sample window for wind measurement), then stops the pulses measurement (disabling the interrupt), calculates the wind speed in this 2,401 seconds window and stores this value in the windSpeed variable. This will used to compose the json string.

## Wind direction measurement
The wind direction measurement is provided by windDirectionReading() function. It reads the analog value from the PIN connected to the Spurkfun Weather Meter Kit Wind Vane component (using the 10k ohm resistor) and converts this raw value in wind direction degree. As describe in the datasheet, a specified voltage value maps a specific wind direction. Therefore the windDirectionReading() function maps the analog raw value to wind direction and return this in degrees value. The analog value is a AVG on 50 consecutive reads. This degree value is stored in the windDir variable and it will used to compose the json string. 

## Rain measurement ![](https://img.shields.io/badge/status-todo-red)
~~The rain measurement is provided by rainReading function. As describe in Spurkfun Weather Meter Kit datasheet, every 0.2794mm of rain causes the switch to close once, then the rain measurement can be executed counting the numbers of switch closed. Due to the External Module go to sleep for a defined time, is necessary to count the rain switch close during the normal mode and during the sleep mode too. To do this, there are 2 different counters:~~
~~- rainCounterDuringSleep~~
~~- rainCounterDuringActive~~

~~It works as following:~~

~~During the normal mode, at startup time, a interrupt function to monitor the rain switch close is enabled. If a rain switch close is detected, the interrupt function increments the rainCounterDuringActive counter, then, when the rainReading function is called, it uses the counter to calculate the rain amount.~~

~~Instead, during the sleep mode, the External module monitors the rain GPIO and if it detects a rain switch close, wake-up the External module, increases the rainCounterDuringSleep counter and executes the normal mode above described.~~

## Battery voltage measurement
As already mentioned the voltages on GPIO33 shifts between 0 and 3,3 volts then between 0 and 4095 values (the ADC pin has 12bit resolution), so we can establish a constant to calculate the voltage applied to the pin based on its value. This constant, theoretically, will be c = 3,3 / 4095 = 0,000805860805861. As we are applying a voltage divider and the voltage applied to the pin is half the voltage of the battery, our constant should be c = 0,000805860805861 * 2 = 0,001611721611722. This means, for each unit in ADC pin we have 0,001611721611722 Volts applied to it.

For example, if the read value on ADC pin is 2320, then the voltage applied to the pin should be VBatt = 2320 * 0,001611721611722 = 3,74V

ADC pins are not that precise, so the value of our constant should be adjusted to a level we consider it is valid for our components. In my case, after doing some testings I have concluded that the best value for the conversion factor is **0.001715**.

sources: https://www.pangodream.es/esp32-getting-battery-charging-level and https://www.settorezero.com/wordpress/arduino-nano-33-iot-wifi-ble-e-imu/


The battery voltage measurement is provided by batteryLevel() function. It reads the analog value from the PIN connected to battery and converts this raw value in voltage measurement using the above conversion factor. The analog value is a AVG on 50 consecutive reads.

The voltage measurement is stored in the volt variable and it will used to compose the json string.

## Json string creation and LoRa sending
When all weather data have been read, these are inserted in a json string using the composeJson() function, then the string is sent to the Gateway using LoRaSend(String packet) function via LoRa connection. After sending the string, the External module restart the cicle.
