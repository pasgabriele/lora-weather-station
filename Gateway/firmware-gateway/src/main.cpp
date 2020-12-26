#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <jsonlib.h>
#include <ArduinoJson.h>

//used digital pins:
#define SCK 5                       //for lora module
#define MISO 19                     //for lora module
#define MOSI 27                     //for lora module
#define SS 18                       //for lora module
#define RST 14                      //for lora module
#define DIO0 26                     //for lora module
#define LED 2

//global variables
StaticJsonDocument<300> data;
int counter = 1;                    //debug
float BMETemperature = -50.0;
float BMEHumidity = 0.0;
float BMEPressure = 0.0;
int id = 0;
int batteryRaw = -1;
float volt = 0.0;
float windSpeed = -1.0;
float windGust = -1.0;
float UVIndex = 0.0;
int windDir = -1;
float rain = -1.0;

//function to connect to lora
void lora_connection() {
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
    delay(500);
  }

  //Change sync word (0xF3) to match the sender
  //The sync word assures you don't get LoRa messages from other LoRa transceivers
  //ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  LoRa.enableCrc();
  Serial.println("INFO: LoRa Initializing OK!!!");
  delay(500);
}

void parseJson(int packetSize){
  //turn on ESP32 onboard LED when receives packet
  digitalWrite(LED, HIGH);
  //print incoming packet size
  Serial.print("INFO: Incoming packet size: ");
  Serial.println(packetSize);

  String received = "";

  //read the incoming packet
  for (int i = 0; i < packetSize; i++) {
    received = received + (char)LoRa.read();
  }

  //print the incoming packet with RSSI
  Serial.print("INFO: Received packet '");
  Serial.print(received);
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());

  //parse the received string using lib/jsonlib.h to extract sensors values
  /*id = jsonExtract(received, "id").toInt();
  volt = jsonExtract(received, "supplyVoltage").toFloat();
  batteryRaw = jsonExtract(received, "batteryRaw").toInt();
  BMETemperature = jsonExtract(received, "outTemp").toFloat();
  BMEHumidity = jsonExtract(received, "outHumidity").toFloat();
  BMEPressure = jsonExtract(received, "pressure").toFloat();
  windGust = jsonExtract(received, "windGust").toFloat();
  windSpeed = jsonExtract(received, "windSpeed").toFloat();
  UVIndex = jsonExtract(received, "UV").toFloat();
  windDir = jsonExtract(received, "windDir").toInt();
  rain = jsonExtract(received, "rain").toFloat();*/

  //paese the received string using ArduinoJson.h library to extract sensors values
  DeserializationError error = deserializeJson(data, received);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  id = data["id"];
  batteryRaw = data["batteryRaw"];
  volt = data["supplyVoltage"];
  BMETemperature = data["outTemp"];
  BMEHumidity = data["outHumidity"];
  BMEPressure = data["pressure"];
  windSpeed = data["windSpeed"];
  windGust = data["windGust"];
  windDir = data["windDir"];
  UVIndex = data["UV"];
  rain = data["rain"];

  Serial.print("INFO: ID mes: ");
  Serial.println(id);
  Serial.print("INFO: Battery RAW: ");
  Serial.println(batteryRaw);
  Serial.print("INFO: Battery: ");
  Serial.println(volt);
  Serial.print("INFO: BME Temp: ");
  Serial.println(BMETemperature);
  Serial.print("INFO: BME Hum: ");
  Serial.println(BMEHumidity);
  Serial.print("INFO: BME Press: ");
  Serial.println(BMEPressure);
  Serial.print("INFO: Wind gust: ");
  Serial.println(windGust);
  Serial.print("INFO: Wind speed: ");
  Serial.println(windSpeed);
  Serial.print("INFO: UV Index: ");
  Serial.println(UVIndex);

  //debug counter
  Serial.print("INFO: Received packet: ");
  Serial.println(counter);
  counter++;
  
  digitalWrite(LED, LOW);
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println();
  Serial.println("Gateway - LoRa weather station by Pasgabriele");

  pinMode(LED, OUTPUT);

  //lora connection
  lora_connection();

  //wifi connection
  //wifi_connection();
}

void loop() {
  // put your main code here, to run repeatedly:
  int packetSize = LoRa.parsePacket();
  if (packetSize){

    parseJson(packetSize);

    //TODO: perform semantic verification on parsed data before sending via MQTT
    //for example, no send MQTT message when External module send json message with:
    //  BMETemperature == null
    //  BMETemperature != [-20;+50]
    //  BMEHumidity == null
    //  BMEHumidity != [0;100]
    //  BMEPressure == null
    //  BMEPressure != [....]
    //  etc...

    //send parsed data to WeeWX via MQTT protocol
    //sendToMQTTBroker();
    //turn off ESP32 onboard LED 
  }
}
