
# External module

The External module is the set of all necessary sensors to detect weather data. It is able to detect the following measures:
- Temperature ![](https://img.shields.io/badge/status-ok-green)
- Humidity ![](https://img.shields.io/badge/status-ok-green)
- Pressure ![](https://img.shields.io/badge/status-ok-green)
- Wind speed ![](https://img.shields.io/badge/status-testing-yellow)
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

## Hardware description

The External module is composed as following:

|Quantity|Name|Description|Alternative|
|--|--|--|--|
|1|[Lilygo SX1278 LoRa ESP32 443Mhz](http://www.lilygo.cn/prod_view.aspx?TypeId=50003&Id=1133&FId=t3:50003:3)|Microcontroller SX1278 ESP32 with LoRa trasmitter at 433Mhz|Equivalent ESP32 with LoRa trasmitter at 433Mhz (for example  [Heltec WiFi LoRa 32](https://heltec.org/project/wifi-lora-32/))|
|1|[TP4056](https://it.aliexpress.com/item/32986135934.html)|Battery charger 5V 1A||
|1|[Solar panel](https://it.aliexpress.com/item/32877897718.html)|6V 750mA solar panel||
|2|[Panasonic 18650](https://it.aliexpress.com/item/4000484192899.html)|Batteries Panasonic 18650 NCR18650B 3.7V 3400mAh Li-Ion with PCB||
|1|[BME280](https://it.aliexpress.com/item/32849462236.html)|Temperature, humidity and preassure sensor||
|1|[Spurkfun Weather Meter Kit](https://www.sparkfun.com/products/15901)|Wind speed, wind direction and rain accumulation sensor||
|1|[VEML6075](https://it.aliexpress.com/item/32843641073.html)|UVA and UVB sensor||
|1|[DS3231](https://it.aliexpress.com/item/32925920564.html)|Real time clock||
