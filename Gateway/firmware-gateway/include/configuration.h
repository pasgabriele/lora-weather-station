#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <WiFi.h>
#include <NTPClient.h>

//constant defined
#define WDT_TIMEOUT 20              //in seconds
const char* SSID = "TO_INSERT";
const char* password =  "TO_INSERT";
const char* MQTTServer = "TO_INSERT";
const char* MQTTUsername = "TO_INSERT";
const char* MQTTPassword = "TO_INSERT";
const char* MQTTTopic = "weather";

//global variables
IPAddress ipaddress(192, 168, 1, 115);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(8, 8, 8, 8);
IPAddress dns2(8, 8, 4, 4);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org");

#endif