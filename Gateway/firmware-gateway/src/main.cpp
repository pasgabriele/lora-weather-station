#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <jsonlib.h>

//define the pins used by the transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

int counter = 1;                //debug
float BMETemperature = -50.0;
float BMEHumidity = 0.0;
float BMEPressure = 0.0;
int id = 0;
float volt = 0;
float windspdkmh_avg10s = 0.0;
float windgustkmh = 0.0;

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
    delay(1000);
  }

  //Change sync word (0xF3) to match the sender
  //The sync word assures you don't get LoRa messages from other LoRa transceivers
  //ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  LoRa.enableCrc();
  Serial.println("INFO: LoRa Initializing OK!!!");
  delay(2000);
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println();
  Serial.println("Gateway - LoRa weather station by Pasgabriele");

  //lora connection
  lora_connection();

  //wifi connection
  //wifi_connection();
}

void loop() {
  // put your main code here, to run repeatedly:
  int packetSize = LoRa.parsePacket();
  if (packetSize){
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

    //parse the received string using lib/jsonlib.h to extract sensors value
    id = jsonExtract(received, "id").toInt();
    volt = jsonExtract(received, "battery").toFloat();
    BMETemperature = jsonExtract(received, "BMETemperature").toFloat();
    BMEHumidity = jsonExtract(received, "BMEHumidity").toFloat();
    BMEPressure = jsonExtract(received, "BMEPressure").toFloat();
    windgustkmh = jsonExtract(received, "windGustKMH").toFloat();
    windspdkmh_avg10s = jsonExtract(received, "windSpdKMH_avg10s").toFloat();

    Serial.print("INFO: ID mes: ");
    Serial.println(id);
    Serial.print("INFO: Battery: ");
    Serial.println(volt);
    Serial.print("INFO: BME Temp: ");
    Serial.println(BMETemperature);
    Serial.print("INFO: BME Hum: ");
    Serial.println(BMEHumidity);
    Serial.print("INFO: BME Press: ");
    Serial.println(BMEPressure);
    Serial.print("INFO: Wind gust: ");
    Serial.println(windgustkmh);
    Serial.print("INFO: AVG Wind speed 10s: ");
    Serial.println(windspdkmh_avg10s);

    //debug counter
    Serial.print("INFO: Received packet: ");
    Serial.println(counter);
    counter++;

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
  }
}
