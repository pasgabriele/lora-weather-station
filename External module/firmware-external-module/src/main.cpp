#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BME280.h>
#include <Adafruit_VEML6075.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

//used digital pins:
#define SCK 5                       //for lora module
#define MISO 19                     //for lora module
#define MOSI 27                     //for lora module
#define SS 18                       //for lora module
#define RST 14                      //for lora module
#define DIO0 26                     //for lora module
#define LED 2                       //for led onboard
#define WSPEED 23                   //for wind sensor

//used analogic pins:
#define BATT 33                     //for battery monitoring
#define WDIR 32                     //for wind sensor

#define WDT_TIMEOUT 60              //in seconds

//debug variable
bool debug = false;
int batteryRaw = 0;

const float WINDSPEED_SCALE = 2.401;    //anemometer coefficient (at 2.401 km/h the anemometer pulse once per second)
const float WINDSPEED_PERIOD = 2.401;   //sample time for wind speed measurement
const float BATTERY_CONV = 0.001715;  //analog to volt constant

//global variables
unsigned long counter = 0;
Adafruit_BME280 bme;
float BMETemperature = -50.0;
float BMEHumidity = -10.0;
float BMEPressure = -10.0;
Adafruit_VEML6075 uv = Adafruit_VEML6075();
float UVIndex = -10.0;
float volt = 0.0;
float windDir = -1;
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;
float windSpeed = -1.0;
bool BMEInitializationStatus = false;
bool UVInitializationStatus = false;


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
    if(debug){
      Serial.println("INFO: Wait LoRa Begin...");
      Serial.println(".");
    }
    delay(500);
  }

  //change sync word (0xF3) to match the receiver
  //the sync word assures you don't get LoRa messages from other LoRa transceivers
  //ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);

  //IMPORTANT: enableCrc() on Gateway too. This will discard wrong packets received on Gateway
  LoRa.enableCrc();
  if(debug){
    Serial.println("INFO: LoRa Initializing OK!");
  }
}

boolean BMEInitialization(){
  //setup BME280
  if(debug){
    Serial.print("INFO: BME Initilizing.");
  }

  for(int i=0; i<5;i++){
    if(bme.begin(0x76)){          //set to 0x77 for some BME280
      if(debug){
        Serial.println();
        Serial.println("INFO: BME280 Initilizing OK!");
      }
      return true;
    }
    if(debug){
      Serial.print(".");
    }
  }
  if(debug){
    Serial.println("ERROR: Couldn't find BME280");
  }
  return false;
}

//function to read BME280 data
void BMEReading(){
  //BME280 read Temperature
  BMETemperature = bme.readTemperature();
  if(debug){
    Serial.print(F("INFO: BME Reading Temperature: "));
    Serial.print(BMETemperature);
    Serial.println(F("°C"));
  }

  //BME280 read humidity
  BMEHumidity = bme.readHumidity();
  if(debug){
    Serial.print(F("INFO: BME Reading Humidity: "));
    Serial.print(BMEHumidity);
    Serial.println(F("%"));
  }

  //BME280 read pressure
  BMEPressure = bme.readPressure()/100.0F;    //from Pascal to mbar (hPa)
  if(debug){
    Serial.print(F("INFO: BME Reading Pressure: "));
    Serial.print(BMEPressure);
    Serial.println(F("mbar"));
  }
}

boolean UVInitialization(){
  //setup VEML6075
  if(debug){
    Serial.print("INFO: VEML6075 Initilizing.");
  }

  for(int i=0; i<5;i++){
    if(uv.begin()){
      if(debug){
        Serial.println();
        Serial.println("INFO: VEML6075 Initilizing OK!");
      }
      return true;
    }
    if(debug){
      Serial.print(".");
    }
  }
  if(debug){
    Serial.println("ERROR: Couldn't find VEML6075");
  }
  return false;
}

//function to read VEML6075 data (UV index)
void UVReading(){
  //VEML6075 read UV index
  UVIndex = uv.readUVI();
  if(debug){
    Serial.print(F("INFO: VEML6075 Reading UV Index: "));
    Serial.println(UVIndex);
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

//function to read battery level
void batteryLevel(){
  pinMode(BATT, INPUT);
  batteryRaw = averageAnalogRead(BATT);
  if(debug){
    Serial.print(F("INFO: Battery analogic pin value: "));
    Serial.print(batteryRaw);
    Serial.println(F("/4095"));
  }

  //mapping analogic value to voltage battery level
  //this convertion is derived by: https://www.pangodream.es/esp32-getting-battery-charging-level
  //this convertion is true for battery wiring in the following schema:
  //        Batt+
  //          |
  //        27kohm
  //          |
  //          +-------GPIO33
  //          |
  //        27kohm
  //          |
  //        Batt-
  volt = batteryRaw * BATTERY_CONV;
  if(debug){
    Serial.print(F("INFO: Battery Voltage: "));
    Serial.print(volt);
    Serial.println(F(" Volt"));
  }
}

//read the wind direction sensor
void windDirectionReading(){
  unsigned int adc;
  pinMode(WDIR, INPUT);
  adc = averageAnalogRead(WDIR); //get the current reading from the sensor

  if      (adc < 150) windDir = 112.5;
  else if (adc < 220) windDir = 67.5;    
  else if (adc < 280) windDir = 90;
  else if (adc < 500) windDir = 157.5;
  else if (adc < 700) windDir = 135;
  else if (adc < 900) windDir = 202.5;   
  else if (adc < 1200) windDir = 180;
  else if (adc < 1600) windDir = 22.5;
  else if (adc < 1900) windDir = 45;
  else if (adc < 2300) windDir = 247.5;
  else if (adc < 2500) windDir = 225;
  else if (adc < 2800) windDir = 337.5;    
  else if (adc < 3100) windDir = 0;
  else if (adc < 3400) windDir = 292.5;
  else if (adc < 3800) windDir = 315;
  else windDir = 270;

  if(debug){
    Serial.print("INFO: Analog value: ");
    Serial.println(adc);
    Serial.print("INFO: Wind Dir degree: ");
    Serial.println(windDir);
  }
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
  windClicks++;
  lastWindIRQ = timeAnemometerEvent; //set up for next event
}

//read the wind speed sensor
void windSpeedReading(){
  pinMode(WSPEED, INPUT); //input from wind meters windspeed sensor

  windClicks = 0;           //reset windClicks count for new calculation
  lastWindIRQ = 0;

  //attach external interrupt pins to IRQ functions
  attachInterrupt(WSPEED, wspeedIRQ, FALLING);
  delay (WINDSPEED_PERIOD * 1000);             //wait 2,401 seconds to average
  //detach external interrupt pins to IRQ functions
  detachInterrupt(WSPEED);

  //as described in Sparkfun Weather Meter Kit (SEN-15901)(https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf),
  //a wind speed of 2.401km/h causes the switch to close once per second.
  //then we can use the following formula:
  //
  //windspeed = 2.401 * #_pulses / interval_time
  //
  windSpeed = windClicks * WINDSPEED_SCALE / WINDSPEED_PERIOD;

  if(windClicks == 0){
    windSpeed = 0;
  }

  if(debug){
    Serial.print(F("INFO: Wind Speed: "));
    Serial.print(windSpeed);
    Serial.println(F(" km/h"));
  }
}

//function to compose json string
String composeJson(){
  StaticJsonDocument<300> data;
  String string;
  //populate JsonFormat
  data["id"] = counter++;
  data["supplyVoltage"] = volt;
  data["batteryRaw"] = batteryRaw;
  data["outTemp"] = BMETemperature;
  data["outHumidity"] = BMEHumidity;
  data["pressure"] = BMEPressure;
  data["windSpeed"] = windSpeed;
  data["windDir"] = windDir;
  data["UV"] = UVIndex;

  //copy JsonFormat to string
  serializeJson(data, string);
  Serial.println("INFO: The following string will be send...");
  Serial.println(string);
  return string;
}

//function to send lora packet
void LoRaSend(String packet){
  //turn on ESP32 onboard LED before LoRa send
  digitalWrite(LED, HIGH);
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
  //turn off ESP32 onboard LED when finished
  digitalWrite(LED, LOW);
}

void setup() {
  pinMode(LED, OUTPUT);

  //Set CPU clock to 80MHz or 26MHz
  setCpuFrequencyMhz(26);

  //enable panic so ESP32 restarts
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);           //add current thread to WDT watch

  //initialize serial monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println();
  Serial.println("External module - LoRa weather station by Pasgabriele");

  lora_connection();

  BMEInitializationStatus = BMEInitialization();
  UVInitializationStatus = UVInitialization();

}

void loop() {
  unsigned int start = millis();

  //read BME280 data
  if (BMEInitializationStatus){
    BMEReading();
  }
  
  //read battery voltage
  batteryLevel();

  //read wind direction
  windDirectionReading();

  //read wind speed
  windSpeedReading();

  //read VEML6075 data
  if(UVInitializationStatus){
    UVReading();
  }

  //send packet to LoRa Receiver using composeJson function as input
  LoRaSend(composeJson());

  unsigned int end = millis();
  unsigned int time = (end - start);
  Serial.print("Cicle time: ");
  Serial.print(time);
  Serial.println(" msec");

  //reset WDT every loop
  esp_task_wdt_reset();
}