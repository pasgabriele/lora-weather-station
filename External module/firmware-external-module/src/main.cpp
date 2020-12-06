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
#define WDIR 35                     //to modify because GPIO35 in ESP32 is output only

#define WIND_DIR_AVG_SIZE 5

//deep sleep configuration
#define uS_TO_S_FACTOR 1000000      //conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  2           //time ESP32 will go to sleep (in seconds) (900 = 15 minutes)

//global variables
Adafruit_BME280 bme;
float BMETemperature = -50.0;
float BMEHumidity = 0.0;
float BMEPressure = 0.0;
float volt = 0.0;
long lastSecond;                    //The millis counter to see when a second rolls by
byte seconds_5s;                   //Keeps track of the "wind speed/dir avg" over last 5 seconds array of data
long lastWindCheck = 0;
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;
int winddiravg[WIND_DIR_AVG_SIZE];  //5 ints to keep track of 5 seconds average
float windspdavg[5];                //array 5 float to keep track of 5s average
float windspeedmph = 0;             //[mph instantaneous wind speed]
float windgustmph = 0;              //[mph current wind gust, using software specific time period]
float windspdmph_avg5s = 0;         //[mph 10 second average wind speed mph]
float windspeedkmh = 0;             //[km/h instantaneous wind speed]
float windgustkmh = 0;              //[km/h current wind gust, using software specific time period]
float windspdkmh_avg10s = 0;        //[km/h 10 second average wind speed km/h]
float windspeedms = 0;              //[m/s instantaneous wind speed]
float windgustms = 0;               //[m/s current wind gust, using software specific time period]
float windspdms_avg10s = 0;         //[m/s 10 second average wind speed m/s]
int winddir = 0;                    //[0-360 instantaneous wind direction]
int windgustdir = 0;                //[0-360 using software specific time period]
int winddir_avg5s = 0;              //[0-360 5 second average wind direction]

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

//Interrupt routines (these are called by the hardware interrupts, not by the main code)
void wspeedIRQ(){
  //Activated by the magnet in the anemometer (2 ticks per rotation), attached to input WSPEED
  if (millis() - lastWindIRQ > 10){ //Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
    lastWindIRQ = millis();         //Grab the current time
    windClicks++;                   //There is 1.492MPH for each click per second.
  }
}

//Calculates each of the variables that wunderground is expecting
void calcWeather(){
  winddir = get_wind_direction();

  //Calc windspdmph_avg5s
  float temp = 0;
  for (int i = 0 ; i < 5 ; i++)
    temp += windspdavg[i];
  temp /= 5.0;
  windspdmph_avg5s = temp;

  //Calc winddir_avg10s, Wind Direction
  //You can't just take the average. Google "mean of circular quantities" for more info
  //We will use the Mitsuta method because it doesn't require trig functions
  //And because it sounds cool.
  //Based on: http://abelian.org/vlf/bearings.html
  //Based on: http://stackoverflow.com/questions/1813483/averaging-angles-again
  long sum = winddiravg[0];
  int D = winddiravg[0];
  for (int i = 1 ; i < WIND_DIR_AVG_SIZE ; i++){
    int delta = winddiravg[i] - D;

    if (delta < -180)
      D += delta + 360;
    else if (delta > 180)
      D += delta - 360;
    else
      D += delta;

    sum += D;
  }
  winddir_avg5s = sum / WIND_DIR_AVG_SIZE;
  if (winddir_avg5s >= 360) winddir_avg5s -= 360;
  if (winddir_avg5s < 0) winddir_avg5s += 360;
}

//Returns the instataneous wind speed
float get_wind_speed(){
  float deltaTime = millis() - lastWindCheck;       //750ms

  deltaTime /= 1000.0;                              //Covert to seconds

  float windSpeed = (float)windClicks / deltaTime;  //3 / 0.750s = 4

  windClicks = 0;                                   //Reset and start watching for new wind
  lastWindCheck = millis();

  windSpeed *= 1.492;                               //4 * 1.492 = 5.968MPH

  /* Serial.println();
    Serial.print("Windspeed:");
    Serial.println(windSpeed);*/
  return (windSpeed);
}

void convertMPHtoKMH(){
  windspeedkmh = windspeedmph * 1.60934;
  windgustkmh = windgustmph * 1.60934;
  windspdkmh_avg10s = windspdmph_avg5s * 1.60934;
}

void convertMPHtoMS(){
  windspeedms = windspeedmph * 0.44704;
  windgustms = windgustmph * 0.44704;
  windspdms_avg10s = windspdmph_avg5s * 0.44704;
}

//Prints the various variables directly to the port
//I don't like the way this function is written but Arduino doesn't support floats under sprintf
void printWeather(){
  calcWeather();                    //Go calc all the various sensors
  convertMPHtoKMH();
  convertMPHtoMS();
}

void readWind(){
  pinMode(WSPEED, INPUT_PULLUP);    //input from wind meters windspeed sensor

  lastSecond = millis();

  //attach external interrupt pins to IRQ functions
  attachInterrupt(WSPEED, wspeedIRQ, FALLING);

  //turn on interrupts
  interrupts();

  long startTime = millis();
  long endTime = millis();
  while(endTime - startTime < 5000){
    //Keep track of which minute it is
    if (millis() - lastSecond >= 1000){
      lastSecond += 1000;

      //Take a speed reading every second for 5 second average
      if (++seconds_5s > 4) seconds_5s = 0;

      //Calc the wind speed and direction every second for 5 second to get 5 second average
      float currentSpeed = get_wind_speed();
      windspeedmph = currentSpeed;          //update global variable for windspeed when using the printWeather() function
      int currentDirection = get_wind_direction();
      windspdavg[seconds_5s] = currentSpeed;
      winddiravg[seconds_5s] = currentDirection;

      //Check to see if this is a gust for the day
      if (currentSpeed > windgustmph)
      {
        windgustmph = currentSpeed;
        windgustdir = currentDirection;
      }
    }
    endTime = millis();
  }
  printWeather();
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
  data["windGustKMH"] = windgustkmh;
  data["windSpdKMH_avg10s"] = windspdkmh_avg10s;

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
