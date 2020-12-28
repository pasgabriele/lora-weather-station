#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>

//used digital pins:
#define SCK 5                       //for lora module
#define MISO 19                     //for lora module
#define MOSI 27                     //for lora module
#define SS 18                       //for lora module
#define RST 14                      //for lora module
#define DIO0 26                     //for lora module
#define LED 2

//constant defined
#define WDT_TIMEOUT 10               //in seconds
const char* SSID = "TOINSERT";
const char* password =  "TOINSERT";
const char* MQTTServer = "TOINSERT";
const char* MQTTUsername = "TOINSERT";
const char* MQTTPassword = "TOINSERT";
const char* MQTTTopic = "weather";

//global variables
StaticJsonDocument<300> data;
String received = "";
IPAddress ipaddress(192, 168, 0, 115);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(8, 8, 8, 8);
IPAddress dns2(8, 8, 4, 4);
WiFiClient WIFIClient;
PubSubClient MQTTClient(WIFIClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
int timestamp = 0;
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
int rxCheckPercent = -1;

//function to connect to lora
void lora_connection() {
  //setup LoRa transceiver module
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS, RST, DIO0);

  //replace the LoRa.begin(---E-) argument with your location's frequency
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  Serial.println("INFO: Wait LoRa Begin...");
  while (!LoRa.begin(433E6)) {
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

//function to connect to WiFi
void wifi_connection() {
  if (!WiFi.config(ipaddress, gateway, subnet, dns1, dns2)) {
    Serial.println("ERROR: STA Failed to configure");
  }

  Serial.print("INFO: Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

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

//function to connect the client to MQTT Broker
void mqtt_connection(){
  MQTTClient.setServer(MQTTServer, 1883);

  while (!MQTTClient.connected()){
    Serial.print("INFO: Connecting to MQTT...");
 
    if (MQTTClient.connect("Gateway", MQTTUsername, MQTTPassword)){
      Serial.println("connected");
    }
    else{
      Serial.print("failed with state ");
      Serial.println(MQTTClient.state());
      delay(500);
    }
  }
}

//function to disconnect the client from MQTT Broker
void mqtt_disconnect(){
  MQTTClient.disconnect();
  Serial.println("INFO: Disconnected from MQTT Server");
}

void sendToMQTTBroker(){
  //connect to MQTT Broker
  mqtt_connection();

  //publish json string to MQTT Broker
  char buffer[256];
  serializeJson(data, buffer);
  MQTTClient.publish(MQTTTopic, buffer);

  //disconnect from MQTT Broker
  mqtt_disconnect();
}

void parseJson(int packetSize){
  //turn on ESP32 onboard LED when receives packet
  digitalWrite(LED, HIGH);
  //print incoming packet size
  Serial.print("INFO: Incoming packet size: ");
  Serial.println(packetSize);

  received = "";

  //read the incoming packet
  for (int i = 0; i < packetSize; i++) {
    received = received + (char)LoRa.read();
  }

  //print the incoming packet with RSSI
  rxCheckPercent = LoRa.packetRssi();
  Serial.print("INFO: Received packet '");
  Serial.print(received);
  Serial.print("' with RSSI ");
  Serial.println(rxCheckPercent);

  //map rssi value to percentage
  rxCheckPercent = map(rxCheckPercent, -145, -30, 0, 100);

  //convert incoming string in json object using ArduinoJson.h library
  DeserializationError error = deserializeJson(data, received);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  //extract sensors values from json object
  id = data["id"];
  volt = data["supplyVoltage"];
  BMETemperature = data["outTemp"];
  BMEHumidity = data["outHumidity"];
  BMEPressure = data["pressure"];
  windSpeed = data["windSpeed"];
  windGust = data["windGust"];
  windDir = data["windDir"];
  UVIndex = data["UV"];
  rain = data["rain"];
  data["rxCheckPercent"] = rxCheckPercent;

  //add timestamp to json string
  timeClient.update();
  timestamp = timeClient.getEpochTime();
  data["dateTime"] = timestamp;

  Serial.print("INFO: Timestamp: ");
  Serial.println(timestamp);
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
  Serial.println(windGust);
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


  //debug counter
  Serial.print("INFO: Received packet: ");
  Serial.println(counter);
  counter++;
  
  //turn off ESP32 onboard LED 
  digitalWrite(LED, LOW);
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
    sendToMQTTBroker();
  }
}
