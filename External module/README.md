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
