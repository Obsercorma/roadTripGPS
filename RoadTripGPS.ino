#include<SoftwareSerial.h>
#include "libs/TinyGPSPlus.h"
#include<SPI.h>
#include<SD.h>
#include<DHT.h>

//#define DEBUG_MODE

/*
 * PINS
 * SD: 
 * MISO: 12
 * MOSI: 11
 * CLK: 13
 * CS: 4
 * 
 * LEDs: red:2 ; green:3 ; blue:10
 * GPS: RX:6 ; TX:5
 * DHT: 7
 * BTNs: step:8 ; start/end:9
 * 
* STATUS CASES:
* RED BLINK: Something went wrong
* PERMANENT RED: GPS Does not receive data properly
* GREEN BLINK: Ending hike
* PERMANENT GREEN: Everything is okay
* BLUE BLINK (2times): Got invalid GPS data
* BLUE BLINK (4times): Step button pressed
* BLUE BLINK (1time 1s): Reach button pressed
*/

#define TIME_INTERVAL 8000
#define TEMP_OFFSET 6.0

void blinkAlert(uint8_t vc, uint16_t d, uint8_t pin, bool sbo=false);

const uint8_t reachBtn = 8;
const uint8_t endBtn = 9;
const uint8_t dhtPin = 7;

const uint8_t redPin = 2;
const uint8_t greenPin = 3;
const uint8_t bluePin = 10;

const uint8_t sdPin = 4;

bool hasResumed = false;
bool valPause = false;
bool valReach = false;
bool endBtnTriggered = false;
uint32_t timeCBtn = 0;

DHT dht(dhtPin, DHT11);
File fileWrite;
SoftwareSerial gpsDevice(6, 5);
TinyGPSPlus gps;
String filename = "dat01.txt";
uint32_t timeCount = 0;
void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  pinMode(endBtn, INPUT_PULLUP);
  pinMode(reachBtn, INPUT_PULLUP);
  
  #ifdef DEBUG_MODE
  Serial.begin(9600);
  #endif
  if (!SD.begin(sdPin)) {
    digitalWrite(redPin, HIGH);
    #ifdef DEBUG_MODE
    Serial.println("initialization failed!");
    #endif
    while (1){
      blinkAlert(3, 200, redPin);
      delay(1000);
    }
  }
  fileWrite = SD.open(filename, FILE_WRITE);
  if(!fileWrite){
    #ifdef DEBUG_MODE
    Serial.println("SD or File not readable!");
    #endif
    while(1) 
    {
      blinkAlert(3, 200, redPin);
      delay(1000);
    }
  }
  dht.begin();
  gpsDevice.begin(9600);
  #ifdef DEBUG_MODE
  Serial.println("Starting");
  #endif
  blinkAlert(1, 200, redPin);
  blinkAlert(1, 200, bluePin);
  blinkAlert(1, 200, greenPin);
}

void blinkAlert(uint8_t vc, uint16_t d, uint8_t pin, bool sbo=false){
  for(int i=0;i<vc;i++){
    digitalWrite(pin, LOW);
    delay(d);
    digitalWrite(pin, HIGH);
    delay(d);
  }
  digitalWrite(pin, sbo);
}

void loop() {
  if(endBtnTriggered){
    blinkAlert(1, 200, greenPin);
    blinkAlert(1, 200, bluePin);
    blinkAlert(1, 200, redPin);
    while(1) delay(1000);
  }
  delay(25);
  bool vr = digitalRead(reachBtn);
  bool ve = digitalRead(endBtn);
  if(ve == LOW){
    timeCBtn = millis();
    do{
      ve = digitalRead(endBtn);
      delay(10);
    }while(ve == LOW);
    if((millis()-timeCBtn) > 2000){
      endBtnTriggered = true;
      fileWrite.println("RT_END");
      fileWrite.close();
      
      #ifdef DEBUG_MODE
      Serial.println("RT_END");
      #endif
      blinkAlert(3, 400, greenPin);
    }
  }
  if(vr == LOW) {
    timeCBtn = millis();
    do{
      vr = digitalRead(reachBtn);
      delay(10);
    }
    while(vr == LOW);
    if((millis()-timeCBtn) > 1500){
      valReach = true;
      blinkAlert(1, 1000, bluePin);
      #ifdef DEBUG_MODE
      Serial.println("VALReach!");
      #endif
    }else{
      blinkAlert(4, 300, bluePin);
      valPause = true;
      #ifdef DEBUG_MODE
      Serial.println("VALPause!");
      #endif
    }
  }
  while (gpsDevice.available() > 0) {
    gps.encode(gpsDevice.read());
  }
  if ((millis() - timeCount) > TIME_INTERVAL)
  {
    timeCount = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if(valPause){
      #ifdef DEBUG_MODE
      Serial.println(hasResumed ? "RT_RESUMED" : "RT_PAUSED");
      #endif
      fileWrite.println(hasResumed ? "RT_R" : "RT_P");
      hasResumed = !hasResumed;
      valPause = false;
      delay(1000);
    }
   if(valReach){
      fileWrite.println("DEST_R");
      valReach = false;
      delay(1000);
    }
    String data = "";
    if (gps.location.isValid() && gps.location.isUpdated()) {
      digitalWrite(greenPin, HIGH);
      data += String(gps.location.lat(), 16) + ";" + String(gps.location.lng(), 16);
    } else {
      digitalWrite(greenPin, LOW);
      blinkAlert(2, 200, bluePin);
      #ifdef DEBUG_MODE
      Serial.println("Invalid GPS Data");
      #endif
      data += "LT_I;LG_I";
    }
    data += ";" + ((gps.date.isValid() && gps.date.isUpdated()) ?
             ((String)gps.date.day() + "/" +
             (String)gps.date.month() + "/" +
             (String)gps.date.year() + "#V")
             : "00/00/2000#NV") + ";" + ((gps.time.isValid() && gps.time.isValid()) ?
             ((String)(gps.time.hour()) + ":" +
             (String)gps.time.minute() + "#V")
             : "00:00#NV");
    fileWrite.print(data);
    #ifdef DEBUG_MODE
    Serial.println(data);
    Serial.print("Tmp:");
    Serial.println((String)dht.computeHeatIndex((t-TEMP_OFFSET), h, false));
    if(isnan(h) || isnan(t)) Serial.println("Invalid DHT11 data!");
    #endif
    fileWrite.println(";" + (!isnan(h) && !isnan(t) ? (String)dht.computeHeatIndex((t-TEMP_OFFSET), h, false) + "#V" : "0.0#NV"));
    #ifdef DEBUG_MODE
    Serial.println("Data written!");
    #endif
  }
}
