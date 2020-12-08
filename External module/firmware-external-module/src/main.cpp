#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>

//used digital pins
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define LED 2
#define WSPEED 13

//used analogic pins
#define BATT 33
#define WDIR 32

//deep sleep configuration
#define uS_TO_S_FACTOR 1000000      //conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  2           //time ESP32 will go to sleep (in seconds) (900 = 15 minutes)

//global variables
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;
Adafruit_BME280 bme;
float BMETemperature = -50.0;
float BMEHumidity = 0.0;
float BMEPressure = 0.0;
float volt = 0.0;
float windSpeed; //wind speed in km/h

//RTC variables. These will be preserved during the deep sleep.
RTC_DATA_ATTR int bootCount = 0;

//function to connect LoRa
void lora_connection(){
  //setup LoRa transceiver module
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS, RST, DIO0);

  //replace the LoRa.begin(---E-) argument with your location's frequency
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(433E6)) {
    Serial.println("INFO: Wait LoRa Begin...");
    Serial.println(".");
    delay(1000);
  }

  //change sync word (0xF3) to match the receiver
  //the sync word assures you don't get LoRa messages from other LoRa transceivers
  //ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);

  //IMPORTANT: enableCrc() on LoRa Receiver too. This will discard wrong packets received on LoRa Receiver
  LoRa.enableCrc();
  Serial.println("INFO: LoRa Initializing OK!");
  delay(2000);
}

//function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason(){
	esp_sleep_wakeup_cause_t wakeup_reason;
	wakeup_reason = esp_sleep_get_wakeup_cause();
	switch(wakeup_reason)
	{
		case ESP_SLEEP_WAKEUP_EXT0  : Serial.println("INFO: Wakeup caused by external signal using RTC_IO"); break;
		case ESP_SLEEP_WAKEUP_EXT1  : Serial.println("INFO: Wakeup caused by external signal using RTC_CNTL"); break;
		case ESP_SLEEP_WAKEUP_TIMER  : Serial.println("INFO: Wakeup caused by timer"); break;
		case ESP_SLEEP_WAKEUP_TOUCHPAD  : Serial.println("INFO: Wakeup caused by touchpad"); break;
		case ESP_SLEEP_WAKEUP_ULP  : Serial.println("INFO: Wakeup caused by ULP program"); break;
		default : Serial.println("INFO: Wakeup was not caused by deep sleep"); break;
	}
}

//Takes an average of readings on a given analog pin
//Returns the average
int averageAnalogRead(int pinToRead){
	byte numberOfReadings = 8;
	unsigned int runningValue = 0;

	for(int x = 0 ; x < numberOfReadings ; x++)
		runningValue += analogRead(pinToRead);
	runningValue /= numberOfReadings;

	return(runningValue);
}

//function to read BME280 data
boolean bmeReading(){
  //setup BME280
  Serial.print("INFO: BME Initilizing.");

  int i = 0;
  while(i < 3){
    if (! bme.begin(0x76)){         //Set to 0x77 for some BME280
      Serial.print(".");
      delay(1000);
    }
    i++;
  }

  if (bme.begin(0x76)){
    Serial.println();
    Serial.println("INFO: BME280 Initilizing OK!");

    //BME280 read Temperature
    BMETemperature = bme.readTemperature();
    Serial.print(F("INFO: BME Reading Temperature: "));
    Serial.print(BMETemperature);
    Serial.println(F("°C"));

    //BME280 read humidity
    BMEHumidity = bme.readHumidity();
    Serial.print(F("INFO: BME Reading Humidity: "));
    Serial.print(BMEHumidity);
    Serial.println(F("%"));

    //BME280 read pressure
    BMEPressure = bme.readPressure()/100.0F;
    Serial.print(F("INFO: BME Reading Pressure: "));
    Serial.print(BMEPressure);
    Serial.println(F("hPa"));

    return true;
  }
  else {
    Serial.println();
    Serial.println("ERROR: Couldn't find BME280");
    return false;
  }
}

//Read the wind direction sensor, return heading in degrees
int get_wind_direction(){
  unsigned int adc;
  float pinVoltage;

  adc = analogRead(WDIR); // get the current reading from the sensor
  pinVoltage = adc * (3.3 / 4096.0);
  /*
  Serial.print("Analogic pin: ");
  Serial.print(adc);
  Serial.println("/4095");
  Serial.print("Pin voltage: ");
  Serial.println(pinVoltage);
  */
  // The following table is ADC readings for the wind direction sensor output, sorted from low to high.
  // Each threshold is the midpoint between adjacent headings. The output is degrees for that ADC reading.
  // Note that these are not in compass degree order! See Weather Meters datasheet for more information.
  //la seguente tabella vale quando il sensore di dir vento è collegato come segue:
  //pin1 rj11 --------> GND
  //pin4 rj11 --+-----> GPIO32
  //            |
  //            +-----> 27k ohm ----->3V3
  //la stazione meteo (external module) dovrà usare una resistenza di 10k ohm quindi la seguente tabelle dovrebbe cambiare mapping
  if ((pinVoltage >= 1.55) && (pinVoltage <= 1.63)) return (0);     //1,59
  if ((pinVoltage >= 0.38) && (pinVoltage <= 0.53)) return (23);    //0.49
  if ((pinVoltage >= 0.55) && (pinVoltage <= 0.65)) return (45);     //0.6
  if (pinVoltage == 0.0) return (90);     //0
  if ((pinVoltage >= 0.10) && (pinVoltage <= 0.15)) return (135);     //0,12
  if ((pinVoltage >= 0.01) && (pinVoltage <= 0.06)) return (158);     //0,03
  if ((pinVoltage >= 0.25) && (pinVoltage <= 0.31)) return (180);     //0,27
  if ((pinVoltage >= 0.18) && (pinVoltage <= 0.23)) return (203);     //0,21
  if ((pinVoltage >= 1.01) && (pinVoltage <= 1.08)) return (225);     //1,04
  if ((pinVoltage >= 0.92) && (pinVoltage <= 0.99)) return (248);     //0,96
  if ((pinVoltage >= 2.40) && (pinVoltage <= 2.55)) return (270);     //2,44
  if ((pinVoltage >= 1.73) && (pinVoltage <= 1.80)) return (293);     //1,77
  if ((pinVoltage >= 2.03) && (pinVoltage <= 2.16)) return (315);     //2,06
  if ((pinVoltage >= 1.25) && (pinVoltage <= 1.35)) return (338);     //1,28
  return (-1); // error, disconnected?
}

//interrupt routines (these are called by the hardware interrupts, not by the main code)
void wspeedIRQ(){
  //activated by the magnet in the anemometer (2 ticks per rotation), attached to input 13
  if (millis() - lastWindIRQ > 10) //ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
  {
    lastWindIRQ = millis(); //grab the current time
    windClicks++; //there is 2.401KMH(1.492MPH) for each click per second.
  }
}

void readWind(){
  pinMode(WSPEED, INPUT_PULLUP); //input from wind meters windspeed sensor

  //attach external interrupt pins to IRQ functions
  attachInterrupt(WSPEED, wspeedIRQ, FALLING);

  windClicks = 0;           //set windClicks count to 0 ready for calculations
  interrupts();             //turn on interrupts
  delay (3000);             //wait 3 seconds to average
  noInterrupts();           //turn off interrupts

  //as described in Sparkfun Weather Meter Kit (SEN-15901)(https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf),
  //a wind speed of 2.401km/h causes the switch to close once per second.
  //then we can use the following formula:
  //
  //windspeed = #_pulses / interval_time * 2,401
  //
  //convert to km/h using the above formula
  //V = P(2.401/3) = P * 0,8
  windSpeed = windClicks * 0.8;

  Serial.print(windClicks);
  Serial.print("\t\t");
  Serial.println(windSpeed);
}

//function to read battery level
void batteryLevel(){
  int analogValue = 0;
  //read analogValue
  analogValue = averageAnalogRead(BATT);
  Serial.print(F("INFO: Analogic Pin Reading: "));
  Serial.print(analogValue);
  Serial.println(F("/4095"));

  //mapping analogic value to voltage battery level
  volt = (analogValue - 0) * (4.2 - 0.0) / (4095 - 0) + 0.0;
  //volt = map(analogValue, 0, 4095, 0.0f, 4.2f);
  Serial.print(F("INFO: Battery Voltage: "));
  Serial.print(volt);
  Serial.println(F("V"));
}

//function to compone json string
String componeJson(){
  StaticJsonDocument<300> data;
  String string;
  //populate JsonFormat
  data["id"] = bootCount;
  data["battery"] = volt;
  data["BMETemperature"] = BMETemperature;
  data["BMEHumidity"] = BMEHumidity;
  data["BMEPressure"] = BMEPressure;
  data["windSpeed"] = windSpeed;

  //copy JsonFormat to string
  serializeJson(data, string);
  Serial.println("INFO: The following string will be send...");
  Serial.println(string);
  return string;
}

//function to send lora packet
void lora_send(String packet){
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
}

void setup() {
  //turn on ESP32 onboard LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  //disable BLE and WiFi
  btStop();
  WiFi.mode(WIFI_MODE_NULL);

  //initialize Serial Monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println();
  Serial.println("External module - LoRa weather station by Pasgabriele");

  //increment boot number and print it every reboot
	++bootCount;
	Serial.println("INFO: Boot number: " + String(bootCount));

  //print the wakeup reason for ESP32
	print_wakeup_reason();

  //lora connection
  lora_connection();

  //read BME280 data
  bmeReading();

  //read battery voltage
  batteryLevel();

  //read windspeed and direction
  readWind();

  //send packet to LoRa Receiver using componeJson function as input
  lora_send(componeJson());

  //set timer to deep sleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  //turn off ESP32 onboard LED
  digitalWrite(LED, LOW);

  //go to deep sleep now
  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop() {
  // put your main code here, to run repeatedly:
}
