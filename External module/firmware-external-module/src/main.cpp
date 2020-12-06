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
#define WSPEED 32

//used analogic pins
#define BATT 33

//deep sleep configuration
#define uS_TO_S_FACTOR 1000000      //conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  2           //time ESP32 will go to sleep (in seconds) (900 = 15 minutes)

//global variables
Adafruit_BME280 bme;
float BMETemperature = -50.0;
float BMEHumidity = 0.0;
float BMEPressure = 0.0;
float volt = 0.0;
long lastSecond; //The millis counter to see when a second rolls by
byte seconds; //When it hits 60, increase the current minute
byte seconds_10s; //Keeps track of the "wind speed/dir avg" over last 10 seconds array of data
long lastWindCheck = 0;
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;

//We need to keep track of the following variables:
//Wind speed each update (no storage)
//Wind gust over the day (no storage)
//Wind speed, avg over 10 seconds (store 1 per second)
//Wind gust over last 10 minutes (store 1 per minute)
byte windspdavg[10]; //10 bytes to keep track of 10s average

//These are all the weather values that wunderground expects:
float windspeedmph = 0; // [mph instantaneous wind speed]
float windgustmph = 0; // [mph current wind gust, using software specific time period]
float windspdmph_avg10s = 0; // [mph 10 second average wind speed mph]
float windspeedkmh = 0; // [km/h instantaneous wind speed]
float windgustkmh = 0; // [km/h current wind gust, using software specific time period]
float windspdkmh_avg10s = 0; // [km/h 10 second average wind speed km/h]
float windspeedms = 0; // [m/s instantaneous wind speed]
float windgustms = 0; // [m/s current wind gust, using software specific time period]
float windspdms_avg10s = 0; // [m/s 10 second average wind speed m/s]

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

//Takes an average of readings on a given pin
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

//Interrupt routines (these are called by the hardware interrupts, not by the main code)
void wspeedIRQ()
// Activated by the magnet in the anemometer (2 ticks per rotation), attached to input 23
{
  if (millis() - lastWindIRQ > 10) // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
  {
    lastWindIRQ = millis(); //Grab the current time
    windClicks++; //There is 1.492MPH for each click per second.
  }
}

//Calculates each of the variables that wunderground is expecting
void calcWeather()
{
  //Calc windspdmph_avg10s
  float temp = 0;
  for (int i = 0 ; i < 10 ; i++)
    temp += windspdavg[i];
  temp /= 10.0;
  windspdmph_avg10s = temp;
}

//Returns the instataneous wind speed
float get_wind_speed()
{
  float deltaTime = millis() - lastWindCheck; //750ms

  deltaTime /= 1000.0; //Covert to seconds

  float windSpeed = (float)windClicks / deltaTime; //3 / 0.750s = 4

  windClicks = 0; //Reset and start watching for new wind
  lastWindCheck = millis();

  windSpeed *= 1.492; //4 * 1.492 = 5.968MPH

  /* Serial.println();
    Serial.print("Windspeed:");
    Serial.println(windSpeed);*/

  return (windSpeed);
}

void convertMPHtoKMH(){
  windspeedkmh = windspeedmph * 1.60934;
  windgustkmh = windgustmph * 1.60934;
  windspdkmh_avg10s = windspdmph_avg10s * 1.60934;
}

void convertMPHtoMS(){
  windspeedms = windspeedmph * 0.44704;
  windgustms = windgustmph * 0.44704;
  windspdms_avg10s = windspdmph_avg10s * 0.44704;
}

//Prints the various variables directly to the port
//I don't like the way this function is written but Arduino doesn't support floats under sprintf
void printWeather(){
  calcWeather(); //Go calc all the various sensors
  convertMPHtoKMH();
  convertMPHtoMS();
}

void readWind(){
  pinMode(WSPEED, INPUT_PULLUP); // input from wind meters windspeed sensor

  seconds = 0;
  lastSecond = millis();

  // attach external interrupt pins to IRQ functions
  attachInterrupt(WSPEED, wspeedIRQ, FALLING);

  // turn on interrupts
  interrupts();

  long startTime = millis();
  long endTime = millis();
  while(endTime - startTime < 10000){
    //Keep track of which minute it is
    if (millis() - lastSecond >= 1000)
    {
      lastSecond += 1000;

      //Take a speed reading every second for 10 second average
      if (++seconds_10s > 10) seconds_10s = 0;

      //Calc the wind speed and direction every second for 10 second to get 10 second average
      float currentSpeed = get_wind_speed();
      windspeedmph = currentSpeed;//update global variable for windspeed when using the printWeather() function
      //float currentSpeed = random(5); //For testing
      windspdavg[seconds_10s] = (int)currentSpeed;
      //if(seconds_10s % 10 == 0) displayArrays(); //For testing

      //Check to see if this is a gust for the day
      if (currentSpeed > windgustmph)
      {
        windgustmph = currentSpeed;
      }
      //Report all readings every second
      printWeather();
    }
    endTime = millis();
  }
}

//function to read battery level
void batteryLevel(){
  int analogValue = 0;
  //read analogValue
  analogValue = averageAnalogRead(BATT);
  Serial.print(F("INFO: Analogic Pin Reading: "));
  Serial.print(analogValue);
  Serial.println(F("/4095"));

  //mapping analogic value to voltage level
  //volt = analogValue;
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
  data["Windgustkmh"] = windgustkmh;
  data["Windspdkmh_avg10s"] = windspdkmh_avg10s;

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
