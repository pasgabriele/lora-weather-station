#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BME280.h>
#include <Adafruit_VEML6075.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <Adafruit_INA219.h>

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
#define WDIR 32                     //for wind sensor

#define WDT_TIMEOUT 60              //in seconds

//debug variable
bool debug = false;

const float WINDSPEED_SCALE = 2.401;    //anemometer coefficient (at 2.401 km/h the anemometer pulse once per second)
const float WINDSPEED_PERIOD = 2.401;   //sample time for wind speed measurement
const float RAIN_SCALE = 0.2794;    //rain bucket coefficient (every 0.2794mm of rain the rain bucket pulse once)

//global variables
unsigned long counter = 0;

Adafruit_BME280 bme;
bool BMEInitializationStatus = false;
float BMETemperature = -50.0;
float BMEHumidity = -10.0;
float BMEPressure = -10.0;

Adafruit_VEML6075 uv = Adafruit_VEML6075();
bool UVInitializationStatus = false;
float UVIndex = -10.0;

float windDir = -1;
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;
float windSpeed = -1.0;

volatile long lastRainIRQ = 0;
volatile byte rainClicks = 0;
float rain = -1.0;

Adafruit_INA219 ina219Batt(0x41);
bool INABattInitializationStatus = false;
float shuntVoltageBatt = 0.0;
float busVoltageBatt = 0.0;
float current_mABatt = 0.0;
float loadVoltageBatt = 0.0;
bool txBatteryStatus = false;
bool charging = false;

Adafruit_INA219 ina219Sol(0x40);
bool INASolInitializationStatus = false;
float shuntVoltageSol = 0.0;
float busVoltageSol = 0.0;
float current_mASol = 0.0;
float loadVoltageSol = 0.0;

//function to connect LoRa
void lora_connection(){
  //setup LoRa transceiver module
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS, RST, DIO0);

  //replace the LoRa.begin(---E-) argument with your location's frequency
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  if(debug){
    Serial.println("INFO: Wait LoRa Begin...");
  }
  while (!LoRa.begin(433E6)) {
    if(debug){
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

boolean INA219BATTInitialization(){
  //setup INA219 BATT
  if(debug){
    Serial.print("INFO: INA219 BATT Initilizing.");
  }

  for(int i=0; i<5;i++){
    if(ina219Batt.begin()){
      if(debug){
        Serial.println();
        Serial.println("INFO: INA219 BATT Initilizing OK!");
      }
      //ina219Batt.setCalibration_16V_400mA();
      ina219Batt.setCalibration_32V_1A();
      return true;
    }
    if(debug){
      Serial.print(".");
    }
  }
  if(debug){
    Serial.println("ERROR: Couldn't find INA219 BATT");
  }
  return false;
}

boolean INA219SOLInitialization(){
  //setup INA219 SOL
  if(debug){
    Serial.print("INFO: INA219 SOL Initilizing.");
  }

  for(int i=0; i<5;i++){
    if(ina219Sol.begin()){
      if(debug){
        Serial.println();
        Serial.println("INFO: INA219 SOL Initilizing OK!");
      }
      //ina219Sol.setCalibration_16V_400mA();
      ina219Sol.setCalibration_32V_1A();
      return true;
    }
    if(debug){
      Serial.print(".");
    }
  }
  if(debug){
    Serial.println("ERROR: Couldn't find INA219 SOL");
  }
  return false;
}

//function to read battery current and voltage
void INABattReading(){
  shuntVoltageBatt = ina219Batt.getShuntVoltage_mV();
  busVoltageBatt = ina219Batt.getBusVoltage_V();
  current_mABatt = ina219Batt.getCurrent_mA();
  loadVoltageBatt = busVoltageBatt + (shuntVoltageBatt / 1000);
  if(debug){
    Serial.print(F("INFO: Battery shunt voltage: "));
    Serial.print(shuntVoltageBatt);
    Serial.println(F(" mV"));
    Serial.print(F("INFO: Battery bus voltage: "));
    Serial.print(busVoltageBatt);
    Serial.println(F(" V"));
    Serial.print(F("INFO: Battery current: "));
    Serial.print(current_mABatt);
    Serial.println(F(" mA"));
  }
  if(loadVoltageBatt > 3.65){
    txBatteryStatus = true;
  }
  else {
    txBatteryStatus = false;
  }
  if(current_mABatt < 0){
    charging = true;
  }
  else{
    charging = false;
  }
}

//function to read solar current and voltage
void INASolReading(){
  shuntVoltageSol = ina219Sol.getShuntVoltage_mV();
  busVoltageSol = ina219Sol.getBusVoltage_V();
  current_mASol = ina219Sol.getCurrent_mA();
  loadVoltageSol = busVoltageSol + (shuntVoltageSol / 1000);
  if(debug){
    Serial.print(F("INFO: Solar shunt voltage: "));
    Serial.print(shuntVoltageSol);
    Serial.println(F(" mV"));
    Serial.print(F("INFO: Solar bus voltage: "));
    Serial.print(busVoltageSol);
    Serial.println(F(" V"));
    Serial.print(F("INFO: Solar current: "));
    Serial.print(current_mASol);
    Serial.println(F(" mA"));
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
    Serial.print("INFO: Wind Dir analogic pin value: ");
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
    Serial.println("INFO: Wind Speed clicks: " + String(windClicks));
    Serial.print(F("INFO: Wind Speed: "));
    Serial.print(windSpeed);
    Serial.println(F(" km/h"));
    Serial.print(F("INFO: Status PIN Wind Speed: "));
    Serial.println(digitalRead(23));
  }
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
	rainClicks++;
  lastRainIRQ = timeRainEvent; //set up for next event
}

void rainReading(){
	//as described in Sparkfun Weather Meter Kit (SEN-15901)(https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf),
  //each 0.2794mm of rain cause one momentary contact closure that can be recorded with a digital counter or microcontroller
	//interrupt input.
  //then we can use the following formula:
  //
  //rain = 0.2794 * #_pulses
  //
	rain = RAIN_SCALE * (rainClicks);
  
  //convert mm in cm
  rain = rain/10;
  if (debug){
    Serial.println("INFO: Rain clicks: " + String(rainClicks));
    Serial.print(F("INFO: Rain: "));
    Serial.print(rain);
    Serial.println(F(" cm"));
    Serial.print(F("INFO: Status PIN Rain: "));
    Serial.println(digitalRead(13));
  }
  rainClicks = 0;
}

//function to compose json string
String composeJson(){
  StaticJsonDocument<300> data;
  String string;
  //populate JsonFormat
  data["id"] = counter++;
  data["sv"] = loadVoltageSol;
  data["cbv"] = loadVoltageBatt;
  data["cs"] = current_mASol;
  data["cb"] = current_mABatt;
  data["ot"] = BMETemperature;
  data["oh"] = BMEHumidity;
  data["p"] = BMEPressure;
  data["ws"] = windSpeed;
  data["wd"] = windDir;
  data["uv"] = UVIndex;
  data["r"] = rain;
  data["tbs"] = txBatteryStatus;
  data["bs1"] = charging;

  //copy JsonFormat to string
  serializeJson(data, string);
  if (debug){
    Serial.println("INFO: The following string will be send...");
    Serial.println(string);
  }
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
  pinMode(RAIN, INPUT); //input from rain gauge sensor

  //Set CPU clock to 80MHz or 26MHz
  setCpuFrequencyMhz(26);

  //enable panic so ESP32 restarts
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);           //add current thread to WDT watch

  //enable interrupt for "online" rain measurement
  attachInterrupt(RAIN, rainIRQ, RISING);

  //initialize serial monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println();
  Serial.println("External module - LoRa weather station by Pasgabriele");

  lora_connection();

  BMEInitializationStatus = BMEInitialization();
  UVInitializationStatus = UVInitialization();

  INABattInitializationStatus = INA219BATTInitialization();
  INASolInitializationStatus = INA219SOLInitialization();
}

void loop() {
  unsigned int start = millis();

  //read BME280 data
  if (BMEInitializationStatus){
    BMEReading();
  }
  
  //read battery voltage and current
  if (INABattInitializationStatus){
    INABattReading();
  }

  //read solar voltage and current
  if (INABattInitializationStatus){
    INASolReading();
  }

  //read wind direction
  windDirectionReading();

  //read wind speed
  windSpeedReading();

  //read VEML6075 data
  if(UVInitializationStatus){
    UVReading();
  }

  //read rain
  rainReading();

  //send packet to LoRa Receiver using composeJson function as input
  LoRaSend(composeJson());

  if(debug){
    unsigned int end = millis();
    unsigned int time = (end - start);
    Serial.print("INFO: Loop time: ");
    Serial.print(time);
    Serial.println(" msec");
  }

  //reset WDT every loop
  esp_task_wdt_reset();
}