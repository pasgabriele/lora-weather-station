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

void onReceive(int packetSize) {
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
  BMETemperature = jsonExtract(received, "BMETemperature").toFloat();
  BMEHumidity = jsonExtract(received, "BMEHumidity").toFloat();
  BMEPressure = jsonExtract(received, "BMEPressure").toFloat();
  id = jsonExtract(received, "id").toInt();
  volt = jsonExtract(received, "battery").toFloat();
  windspdkmh_avg10s = jsonExtract(received, "indspdkmh_avg10s").toFloat();
  windgustkmh = jsonExtract(received, "windgustkmh").toFloat();
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
  Serial.print("INFO: AVG Wind speed 10s: ");
  Serial.println(windspdkmh_avg10s);
  Serial.print("INFO: Wind gust: ");
  Serial.println(windgustkmh);

  //TODO: verificare attendibilita' dati parsati prima di invio a MQTT.
  //      Se 0 scarta, se -50 scarta, se BMEAltitude == 44330 scarta

  //invia i dati letti via mqtt a weewx
  //sendToMQTTBroker();

  //debug counter
  Serial.print("INFO: Received packet: ");
  Serial.println(counter);
  counter++;
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

  //TODO: inviare un messaggio MQTT, contenente tutti i valori ricevuti dai sensori e memorizzati nell'oggetto Json, al broker MQTT.

  //register the receive callback
  LoRa.onReceive(onReceive);

  //put the radio into receive mode
  LoRa.receive();
}

void loop() {
  // put your main code here, to run repeatedly:
}
