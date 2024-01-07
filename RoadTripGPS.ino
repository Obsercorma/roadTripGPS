#include<SoftwareSerial.h>
#include<TinyGPSPlus.h>
#include<SPI.h>
#include<SD.h>
#include<DHT.h>

#define TIME_INTERVAL 8000
#define ALERT_PIN 8

//const uint8_t pausePin = 10;
const uint8_t reachDestPin = 9;
bool hasResumed = false;
bool valPause = false;
bool valReach = false;
bool stateAlertPin = false;
uint32_t timeCBtn = 0;

DHT dht(7, DHT11);
File fileWrite;
SoftwareSerial gpsDevice(6, 5);
TinyGPSPlus gps;
String filename = "dat01.txt";
uint32_t timeCount = 0;
void setup() {
  pinMode(ALERT_PIN, OUTPUT);
  //pinMode(pausePin, INPUT_PULLUP);
  pinMode(reachDestPin, INPUT_PULLUP);
  Serial.begin(9600);
  if (!SD.begin(4)) {
    digitalWrite(ALERT_PIN, HIGH);
    Serial.println("initialization failed!");
    while (1);
  }
  dht.begin();
  gpsDevice.begin(9600);
  Serial.println("Starting");
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
    if((millis()-timeCBtn) > 1000){
      valReach = true;
      blinkAlert(2);
      Serial.println("VALReach!");
    }else{
      blinkAlert(4);
      valPause = true;
      Serial.println("VALPause!");
    }
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
        fileWrite.println(hasResumed ? "RT_PAUSED" : "RT_RESUMED");
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
      double testVar = 48.009842666666664;
      //dtostrf(testVar,17,10,testData);
      //Serial.println(String(testVar*100.0,12));
      fileWrite.close();
    } else {
      //Serial.println("Cannot write into file!");
    }
    //}
  }
}
