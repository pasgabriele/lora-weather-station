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
#define WSPEED 23

//used analogic pins
#define BATT 33

//deep sleep configuration
#define uS_TO_S_FACTOR 1000000      //conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  10           //time ESP32 will go to sleep (in seconds) (900 = 15 minutes)

//global variables
Adafruit_BME280 bme;
float BMETemperature = -50.0;
float BMEHumidity = 0.0;
float BMEPressure = 0.0;
float volt = 0.0;

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

//function to read BME280 data
boolean bmeReading(){
  //setup BME280
  Serial.print("INFO: BME Initilizing.");

  int i = 0;
  while(i < 3){
    if (! bme.begin(0x76)) {            // Set to 0x77 for some BME280
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

//function to read battery level
void batteryLevel(){
  int analogValue = 0;
  //read analogValue
  analogValue = analogRead(BATT);
  Serial.print(F("INFO: Analogic Pin Reading: "));
  Serial.print(analogValue);
  Serial.println(F("/4095"));

  //mapping analogic value to voltage level
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

  btStop();
  WiFi.mode( WIFI_MODE_NULL );

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
