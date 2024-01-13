#include<SoftwareSerial.h>
#include<TinyGPSPlus.h>
#include<SPI.h>
#include<SD.h>
#include<DHT.h>

//#define DEBUG_MODE

#define TIME_INTERVAL 8000
#define ALERT_PIN 8

void blinkAlert(uint8_t vc);

//const uint8_t pausePin = 10;
const uint8_t reachDestPin = 9;
const uint8_t dhtPin = 7;
bool hasResumed = false;
bool valPause = false;
bool valReach = false;
bool stateAlertPin = false;
uint32_t timeCBtn = 0;

DHT dht(dhtPin, DHT11);
File fileWrite;@
SoftwareSerial gpsDevice(6, 5); // RX, TX
TinyGPSPlus gps;
String filename = "dat01.txt";
uint32_t timeCount = 0;
void setup() {
  pinMode(ALERT_PIN, OUTPUT);
  //pinMode(pausePin, INPUT_PULLUP);
  pinMode(reachDestPin, INPUT_PULLUP);
  #ifdef DEBUG_MODE
  Serial.begin(9600);
  #endif
  if (!SD.begin(4)) {
    digitalWrite(ALERT_PIN, HIGH);
    #ifdef DEBUG_MODE
    Serial.println("initialization failed!");
    #endif
    while (1){
      blinkAlert(3);
      delay(1000);
    }
  }
  dht.begin();
  gpsDevice.begin(9600);
  #ifdef DEBUG_MODE
  Serial.println("Starting");
  #endif
}

void blinkAlert(uint8_t vc){
  for(int i=0;i<vc;i++){
    digitalWrite(ALERT_PIN, LOW);
    delay(200);
    digitalWrite(ALERT_PIN, HIGH);
    delay(200);
  }
  digitalWrite(ALERT_PIN, stateAlertPin);
}

void loop() {
  delay(25);
  bool va = digitalRead(reachDestPin);
  if(va == LOW) {
    timeCBtn = millis();
    do{
      va = digitalRead(reachDestPin);
    }
    while(va == LOW);
    #ifdef DEBUG_MODE
    if((millis()-timeCBtn) > 1000){
      valReach = true;
      blinkAlert(4);
      Serial.println("VALReach!");
    }else{
      blinkAlert(2);
      valPause = true;
      Serial.println("VALPause!");
    }
    #else
    if((millis()-timeCBtn) > 1000){
      valReach = true;
      blinkAlert(4);
    }else{
      blinkAlert(2);
      valPause = true;
    }
    #endif
  }
  while (gpsDevice.available() > 0) {
    gps.encode(gpsDevice.read());
  }
  if ((millis() - timeCount) > TIME_INTERVAL)
  {
    timeCount = millis();
    //if(gps.location.isValid()){
    fileWrite = SD.open(filename, FILE_WRITE);
    if (fileWrite) {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      if(valPause){
        #ifdef DEBUG_MODE
        Serial.println(hasResumed ? "RT_RESUMED" : "RT_PAUSED");
        #endif
        fileWrite.println(hasResumed ? "RT_RESUMED" : "RT_PAUSED");
        hasResumed = !hasResumed;
        valPause = false;
        delay(1000);
      }
     if(valReach){
        fileWrite.println("DEST_REACHED");
        valReach = false;
        delay(1000);
      }
      if (gps.location.isValid() && gps.location.isUpdated()) {
        digitalWrite(ALERT_PIN, (stateAlertPin=LOW));
        fileWrite.print(String(gps.location.lat(), 16));
        fileWrite.print(";" + String(gps.location.lng(), 16));
      } else {
        digitalWrite(ALERT_PIN, (stateAlertPin=HIGH));
        fileWrite.print("LAT_INVA;LNG_INVA");
      }
      fileWrite.print(";" + ((gps.altitude.isValid() && gps.altitude.isUpdated()) ? (String)gps.altitude.meters() : "ALT_INV"));
      fileWrite.print(
        ";" + ((gps.date.isValid() && gps.date.isUpdated()) ?
               (String)gps.date.day() + "/" +
               (String)gps.date.month() + "/" +
               (String)gps.date.year() + "#V"
               : "00/00/2000#NV"));
      fileWrite.print(
        ";" + ((gps.time.isValid() && gps.time.isValid()) ?
               (String)(gps.time.hour()) + ":" +
               (String)gps.time.minute() + "#V"
               : "00:00#NV"));
      fileWrite.println(";" + (!isnan(h) && !isnan(t) ? (String)dht.computeHeatIndex(t, h, false) + "#V" : "0.0#NV"));
      Serial.println("Data written!");
      //char testData[20];
      //double testVar = 48.009842666666664;
      fileWrite.close();
    }
  }
}
