#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_VEML6075.h>

//used digital pins:
#define SCK 5                       //for lora module
#define MISO 19                     //for lora module
#define MOSI 27                     //for lora module
#define SS 18                       //for lora module
#define RST 14                      //for lora module
#define DIO0 26                     //for lora module
#define LED 2                       //for led onboard
#define WSPEED 23                   //for wind sensor
#define RAIN 13                     //for rain sensor

//used analogic pins:
#define BATT 33                     //for battery monitoring
#define WDIR 32                     //for wind sensor

//constant defined
#define uS_TO_S_FACTOR 1000000      //conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  1            //time ESP32 will go to sleep (in seconds) (300 = 5 minutes)
const float WINDSPEED_SCALE = 2.401;//anemometer coefficient (at 2.401 km/h the anemometer pulse once per second)
const float WINDSPEED_PERIOD = 5.0; //sample time for wind speed measurement
const float RAIN_SCALE = 0.2794;    //rain bucket coefficient (every 0.2794mm of rain the rain bucket pulse once)

//global variables
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;
unsigned int lastRainIRQ = 0;
volatile unsigned int gustPeriod = UINT_MAX;
int rainCounterDuringSleep = 0;
int rainCounterDuringActive = 0;
float BMETemperature = -50.0;
float BMEHumidity = 0.0;
float BMEPressure = 0.0;
float UVIndex = -1.0;
float volt = 0.0;
float windSpeed = -1.0;
float gustSpeed = -1.0;
int windDir = -1;
float rain = -1.0;
Adafruit_BME280 bme;
Adafruit_VEML6075 uv = Adafruit_VEML6075();

//RTC variables. These will be preserved during the deep sleep.
RTC_DATA_ATTR unsigned long bootCount = 0;

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
    delay(500);
  }

  //change sync word (0xF3) to match the receiver
  //the sync word assures you don't get LoRa messages from other LoRa transceivers
  //ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);

  //IMPORTANT: enableCrc() on Gateway too. This will discard wrong packets received on Gateway
  LoRa.enableCrc();
  Serial.println("INFO: LoRa Initializing OK!");
  delay(500);
}

//function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason(){
	esp_sleep_wakeup_cause_t wakeup_reason;
	wakeup_reason = esp_sleep_get_wakeup_cause();
	switch(wakeup_reason)
	{
		case ESP_SLEEP_WAKEUP_EXT0  : Serial.println("INFO: Wakeup caused by external signal using RTC_IO"); rainCounterDuringSleep++; break;
		case ESP_SLEEP_WAKEUP_EXT1  : Serial.println("INFO: Wakeup caused by external signal using RTC_CNTL"); break;
		case ESP_SLEEP_WAKEUP_TIMER  : Serial.println("INFO: Wakeup caused by timer"); break;
		case ESP_SLEEP_WAKEUP_TOUCHPAD  : Serial.println("INFO: Wakeup caused by touchpad"); break;
		case ESP_SLEEP_WAKEUP_ULP  : Serial.println("INFO: Wakeup caused by ULP program"); break;
		default : Serial.println("INFO: Wakeup was not caused by deep sleep"); break;
	}
}

//takes an average of readings on a given analog pin
//returns the average
int averageAnalogRead(int pinToRead){
	byte numberOfReadings = 50;
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
    if (! bme.begin(0x76)){         //set to 0x77 for some BME280
      Serial.print(".");
      delay(500);
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

//function to read VEML6075 data (UV index)
boolean UVReading(){
  //setup VEML6075
  Serial.print("INFO: VEML6075 Initilizing.");

  int i = 0;
  while(i < 3){
    if (! uv.begin()){
      Serial.print(".");
      delay(500);
    }
    i++;
  }

  if (uv.begin()){
    Serial.println();
    Serial.println("INFO: VEML6075 Initilizing OK!");

    //VEML6075 read UV index
    UVIndex = uv.readUVI();
    Serial.print(F("INFO: VEML6075 Reading UV Index: "));
    Serial.println(UVIndex);

    return true;
  }
  else {
    Serial.println();
    Serial.println("ERROR: Couldn't find VEML6075");

    return false;
  }
}

//read the wind direction sensor, return heading in degrees
int windDirectionReading(){
  unsigned int adc;
  float pinVoltage;

  adc = averageAnalogRead(WDIR); //get the current reading from the sensor
  pinVoltage = adc * (3.3 / 4096.0);
  
  Serial.print("INFO: Wind Dir analogic pin value: ");
  Serial.print(adc);
  Serial.println("/4095");
  Serial.print("INFO: Wind Dir analogic pin voltage: ");
  Serial.print(pinVoltage);
  Serial.println(" Volt");
  
  //the following table is ADC readings for the wind direction sensor output, sorted from low to high.
  //each threshold is the midpoint between adjacent headings. The output is degrees for that ADC reading.
  //note that these are not in compass degree order! See Weather Meters datasheet for more information.
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
  //activated by the magnet in the anemometer (2 ticks per rotation), attached to input 23
  unsigned int timeAnemometerEvent = millis(); // grab current time
  //calculate time since last event
  unsigned int period = timeAnemometerEvent - lastWindIRQ;
  //ignore switch-bounce glitches less than 10mS after initial edge (which implies a max windspeed of 149mph)
  if(period < 10) {
    return;
  }
  //if there's never been an event before (first time through), then just capture it
  if(lastWindIRQ != 0) {
    if(period < gustPeriod) {
      //ff the period is the shortest (and therefore fastest windspeed) seen, capture it
      gustPeriod = period;
    }
  }
  windClicks++;
  lastWindIRQ = timeAnemometerEvent; //set up for next event
}

//interrupt routines (these are called by the hardware interrupts, not by the main code)
void rainIRQ(){
	//count rain gauge bucket tips as they occur
  //activated by the magnet and reed switch in the rain gauge, attached to GPIO13
  unsigned int timeRainEvent = millis(); //grab current time

  //ignore switch-bounce glitches less than 0.5 sec after initial edge
  if(timeRainEvent - lastRainIRQ < 500) {
    return;
  }
	rainCounterDuringActive++;
  lastRainIRQ = timeRainEvent; //set up for next event
}

void windReading(){
  pinMode(WSPEED, INPUT_PULLUP); //input from wind meters windspeed sensor

  windClicks = 0;           //reset windClicks count for new calculation
  gustPeriod = UINT_MAX;    //reset gustPeriod  for new calculation
  lastWindIRQ = 0;

  //attach external interrupt pins to IRQ functions
  attachInterrupt(WSPEED, wspeedIRQ, FALLING);
  delay (WINDSPEED_PERIOD * 1000);             //wait 5 seconds to average
  //detach external interrupt pins to IRQ functions
  detachInterrupt(WSPEED);

  //as described in Sparkfun Weather Meter Kit (SEN-15901)(https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf),
  //a wind speed of 2.401km/h causes the switch to close once per second.
  //then we can use the following formula:
  //
  //windspeed = 2.401 * #_pulses / interval_time
  //
  windSpeed = windClicks * WINDSPEED_SCALE / WINDSPEED_PERIOD;

  //wind gust is:
  gustSpeed = WINDSPEED_SCALE * 1000.0 / float(gustPeriod);

  if(windClicks == 1){
    gustSpeed = windSpeed;
  }

  if(windClicks == 0){
    gustSpeed = 0;
    windSpeed = 0;
  }

  windDir = windDirectionReading();

  Serial.print(F("INFO: Wind Speed: "));
  Serial.print(windSpeed);
  Serial.println(F(" km/h"));
  Serial.print(F("INFO: Gust Speed: "));
  Serial.print(gustSpeed);
  Serial.println(F(" km/h"));
  Serial.print(F("INFO: Wind Direction: "));
  Serial.print(windDir);
  Serial.println(F(" degrees"));
}

void rainReading(){
	//as described in Sparkfun Weather Meter Kit (SEN-15901)(https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf),
  //each 0.2794mm of rain cause one momentary contact closure that can be recorded with a digital counter or microcontroller
	//interrupt input.
  //then we can use the following formula:
  //
  //rain = 0.2794 * #_pulses
  //
	rain = RAIN_SCALE * (rainCounterDuringSleep+rainCounterDuringActive);
	Serial.println("INFO: Rain counter in active mode: " + String(rainCounterDuringActive));
	Serial.println("INFO: Rain counter in sleep mode: " + String(rainCounterDuringSleep));
	Serial.println("INFO: Total rain counter: " + String(rainCounterDuringSleep+rainCounterDuringActive));
	Serial.print(F("INFO: Rain: "));
  Serial.print(rain);
  Serial.println(F(" mm"));
	rainCounterDuringActive = 0;
	rainCounterDuringSleep = 0;
}

//function to read battery level
void batteryLevel(){
  //int analogValue = 0;
  //read analogValue
  pinMode(BATT, INPUT);
  int batteryRaw = averageAnalogRead(BATT);
  Serial.print(F("INFO: Battery analogic pin value: "));
  Serial.print(batteryRaw);
  Serial.println(F("/4095"));

  //mapping analogic value to voltage battery level
  volt = (batteryRaw - 0) * (4.20 - 0.0) / (4095 - 0) + 0.0;
  Serial.print(F("INFO: Battery Voltage: "));
  Serial.print(volt);
  Serial.println(F(" Volt"));
}

//function to compose json string
String composeJson(){
  StaticJsonDocument<300> data;
  String string;
  //populate JsonFormat
  data["id"] = bootCount;
  data["supplyVoltage"] = volt;
  data["outTemp"] = BMETemperature;
  data["outHumidity"] = BMEHumidity;
  data["pressure"] = BMEPressure;
  data["windSpeed"] = windSpeed;
  data["windGust"] = gustSpeed;
  data["windDir"] = windDir;
  data["UV"] = UVIndex;
  data["rain"] = rain;

  //copy JsonFormat to string
  serializeJson(data, string);
  Serial.println("INFO: The following string will be send...");
  Serial.println(string);
  return string;
}

//function to send lora packet
void LoRaSend(String packet){
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
}

void setup() {
  //turn on ESP32 onboard LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  //set mode rain GPIO
  pinMode(RAIN, INPUT_PULLUP);

  //initialize serial monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println();
  Serial.println("External module - LoRa weather station by Pasgabriele");

  //enable interrupt for "online" rain measurement
  attachInterrupt(RAIN, rainIRQ, RISING);

  //increment boot number and print it every reboot
	++bootCount;
	Serial.println("INFO: Boot number: " + String(bootCount));

  //print the wakeup reason for ESP32
	print_wakeup_reason();

  //lora connection
  lora_connection();

  //read BME280 data
  bmeReading();

  //read VEML6075 data
  UVReading();

  //read battery voltage
  batteryLevel();

  //read windspeed and direction
  windReading();

  //read rain
  rainReading();

  //send packet to LoRa Receiver using composeJson function as input
  LoRaSend(composeJson());

  //set wakeup for timer
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  //set wakeup for rain count
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);

  //turn off ESP32 onboard LED
  digitalWrite(LED, LOW);

  //go to deep sleep now
  Serial.println("Going to sleep now");
  delay(500);
  Serial.flush();
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop() {
  // put your main code here, to run repeatedly:
}
