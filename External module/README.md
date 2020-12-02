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

then goes in Deep Sleep mode for a configurable time (default is 15 minutes). Of couse, the batteries saving necessity affects the weather data update frequency. With 15 minutes of Deep Sleep mode you cannot have a real time weather status. This is insignificant for some weather data (for example temperature, humdity and preassure), but it could be significant for other weather data (for example wind speed and direction). For this reason you can set the Deep Sleep time based on your necessities.ties.
