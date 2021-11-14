#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <PubSubClient.h>
#include "configuration.h"

//used digital pins:
#define SCK 5                       //for lora module
#define MISO 19                     //for lora module
#define MOSI 27                     //for lora module
#define SS 18                       //for lora module
#define RST 14                      //for lora module
#define DIO0 26                     //for lora module
#define LED 2

//debug variable
bool debug = true;

//global variables
String received = "";
WiFiClient WIFIClient;
PubSubClient MQTTClient(WIFIClient);
int timestamp = 0;
int counter = 1;                    //debug
float BMETemperature = -50.0;
float BMEHumidity = 0.0;
float BMEPressure = 0.0;
unsigned long id = 0;
float windSpeed = -1.0;
float windGust = -1.0;
float UVIndex = 0.0;
float windDir = -1.0;
float rain = -1.0;
int rxCheckPercent = -1;
float solVolt = 0.0;
float solAmp = 0.0;
float battVolt = 0.0;
float battAmp = 0.0;
bool txBatteryStatus = false;
bool batteryStatus1 = false;

//function to connect to lora
void lora_connection() {
  //setup LoRa transceiver module
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS, RST, DIO0);

  //replace the LoRa.begin(---E-) argument with your location's frequency
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  if(debug)
    Serial.println("INFO: Wait LoRa Begin...");
  while (!LoRa.begin(433E6)) {
    if(debug)
      Serial.println(".");
    delay(500);
  }

  //Change sync word (0xF3) to match the sender
  //The sync word assures you don't get LoRa messages from other LoRa transceivers
  //ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  LoRa.enableCrc();
  if(debug)
    Serial.println("INFO: LoRa Initializing OK!!!");
  delay(500);
}

//function to connect to WiFi
void wifi_connection() {
  digitalWrite(LED, HIGH);
  if (!WiFi.config(ipaddress, gateway, subnet, dns1, dns2)) {
    if(debug)
      Serial.println("ERROR: STA Failed to configure");
  }

  if(debug){
    Serial.print("INFO: Connecting to ");
    Serial.println(SSID);
  }

  WiFi.begin(SSID, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if(debug)
      Serial.print(".");
  }

  if(debug){
    Serial.println("");
    Serial.println("INFO: WiFi connected!!!");
    Serial.print("INFO: IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("INFO: ESP Mac Address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("INFO: Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("INFO: Gateway IP: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("INFO: DNS: ");
    Serial.println(WiFi.dnsIP());
  }
  digitalWrite(LED, LOW);
}

boolean parseJson(int packetSize){

  if(debug){
    //print incoming packet size
    Serial.print("INFO: Incoming packet size: ");
    Serial.println(packetSize);
  }

  received = "";

  //read the incoming packet
  for (int i = 0; i < packetSize; i++) {
    received = received + (char)LoRa.read();
  }

  //print the incoming packet with RSSI
  rxCheckPercent = LoRa.packetRssi();
  if(debug){
    Serial.print("INFO: Received packet '");
    Serial.print(received);
    Serial.print("' with RSSI ");
    Serial.println(rxCheckPercent);
  }

  //map rssi value to percentage
  rxCheckPercent = map(rxCheckPercent, -145, -30, 0, 100);

  StaticJsonDocument<300> data;

  //convert incoming string in json object using ArduinoJson.h library
  DeserializationError error = deserializeJson(data, received);
  if (error) {
    if(debug){
      Serial.print(F("ERROR: deserializeJson() failed: "));
      Serial.println(error.f_str());
    }
    return false;
  }

  //extract sensors values from json object
  id = data["id"];
  solVolt = data["sv"];
  battVolt = data["cbv"];
  solAmp = data["cs"];
  battAmp = data["cb"];
  BMETemperature = data["ot"];
  BMEHumidity = data["oh"];
  BMEPressure = data["p"];
  windSpeed = data["ws"];
  windDir = data["wd"];
  UVIndex = data["uv"];
  rain = data["r"];
  txBatteryStatus = data["tbs"];
  batteryStatus1 = data["bs1"];

  if(debug){
    Serial.print("INFO: ID mes: ");
    Serial.println(id);
    Serial.print("INFO: BME Temp: ");
    Serial.println(BMETemperature);
    Serial.print("INFO: BME Hum: ");
    Serial.println(BMEHumidity);
    Serial.print("INFO: BME Press: ");
    Serial.println(BMEPressure);
    Serial.print("INFO: Wind speed: ");
    Serial.println(windSpeed);
    Serial.print("INFO: Wind dir: ");
    Serial.println(windDir);
    Serial.print("INFO: UV Index: ");
    Serial.println(UVIndex);
    Serial.print("INFO: Rain: ");
    Serial.println(rain);
    Serial.print("INFO: Signal quality: ");
    Serial.println(rxCheckPercent);
    Serial.print("INFO: Solar voltage: ");
    Serial.println(solVolt);
    Serial.print("INFO: Solar current: ");
    Serial.println(solAmp);
    Serial.print("INFO: Battery voltage: ");
    Serial.println(battVolt);
    Serial.print("INFO: Battery current: ");
    Serial.println(battAmp);
    Serial.print("INFO: Battery voltage > 3.65: ");
    Serial.println(txBatteryStatus);
    Serial.print("INFO: Battery in charging: ");
    Serial.println(batteryStatus1);

    //debug counter
    Serial.print("INFO: Received packet: ");
    Serial.println(counter);
  }
  counter++;
  
  return true;
}

//function to connect the client to MQTT Broker
void mqtt_connection(){
  MQTTClient.setServer(MQTTServer, MQTTPort);

  while (!MQTTClient.connected()){
    if(debug)
      Serial.print("INFO: Connecting to MQTT...");
 
    if (MQTTClient.connect("Gateway", MQTTUsername, MQTTPassword)){
      if(debug)
        Serial.println("connected");
    }
    else{
      if(debug){
        Serial.print("ERROR: failed with state ");
        Serial.println(MQTTClient.state());
      }
      delay(500);
    }
  }
}

//function to disconnect the client from MQTT Broker
void mqtt_disconnect(){
  MQTTClient.disconnect();
  if(debug)
    Serial.println("INFO: Disconnected from MQTT Server");
}

boolean sendToMQTTBroker(){
  //connect to MQTT Broker
  mqtt_connection();

  //insert sensor values on json object
  StaticJsonDocument<300> data;
  data["sv"] = solVolt;
  data["cbv"] = battVolt;
  data["cs"] = solAmp;
  data["cb"] = battAmp;
  data["ot"] = BMETemperature;
  data["oh"] = BMEHumidity;
  data["p"] = BMEPressure;
  data["ws"] = windSpeed;
  data["wd"] = windDir;
  data["uv"] = UVIndex;
  data["r"] = rain;
  data["tbs"] = txBatteryStatus;
  data["bs1"] = batteryStatus1;
  data["rcp"] = rxCheckPercent;

  //add timestamp to json string
  while(timeClient.update() == false){
    if(debug)
      Serial.println("WARN: Waiting time from NTP Server");
  }
  timestamp = timeClient.getEpochTime();
  
  data["dt"] = timestamp;

  //publish json string to MQTT Broker
  char buffer[256];
  serializeJson(data, buffer);
  
  size_t len = strlen(buffer);
  if(debug){
    Serial.print("INFO: MQTT array size: ");
    Serial.println(len);
  }
  
  if(MQTTClient.publish(MQTTTopic, buffer)){
    if(debug)
      Serial.println("INFO: MQTT packet sent");
    //disconnect from MQTT Broker
    mqtt_disconnect();
    return true;
  }
  else{
    if(debug)
      Serial.println("ERROR: MQTT packet NOT sent");
    //disconnect from MQTT Broker
    mqtt_disconnect();
    return false;
  }
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println();
  Serial.println("Gateway - LoRa weather station by Pasgabriele");

  //enable panic so ESP32 restarts
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);           //add current thread to WDT watch

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  //lora connection
  lora_connection();

  //wifi connection
  wifi_connection();

  //configure NTP
  timeClient.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  int packetSize = LoRa.parsePacket();
  if (packetSize){
    //reset WDT every loop
    esp_task_wdt_reset();

    //turn on ESP32 onboard LED when receives packet
    digitalWrite(LED, HIGH);

    if(parseJson(packetSize)){
      //send parsed data to WeeWX via MQTT protocol
      sendToMQTTBroker();
    }

    //turn off ESP32 onboard LED when finish
    digitalWrite(LED, LOW);
  }
}
