//LIB
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include "FirebaseESP8266.h"
#include <DHT.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>
#include <Servo.h>

//ESP8266/FireBase config
#define WIFI_SSID "Đặng Thái Hòa"
#define WIFI_PASSWORD "12345678a"

// #define WIFI_SSID "Home 2.4Ghz"
// #define WIFI_PASSWORD "homecafe24"

#define FIREBASE_HOST "homecontrol-60d7d-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "30OHsxJ78Z75ggJgcSI0Zd3COc3E55h6SdiSssuN"

FirebaseData FBData;

String path = "/";
FirebaseJson json;

//DHT config
#define DHTPIN 5 //D1    
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);

//WaterSensor config
#define WATERSENSORPIN A0    

//DS1302 config
// CONNECTIONS:
// DS1302 CLK/SCLK --> D2
// DS1302 DAT/IO --> D3
// DS1302 RST/CE --> D4
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND
ThreeWire myWire(D3,D2,D4); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
#define countof(a) (sizeof(a) / sizeof(a[0]))

//Servo config
Servo Roofservo;

//Led config
int Led[] = {12,13,15};//D6/D7/D8

//Var
float Hum; 
float Temp;

int WaterData;

char datestring[20];
char timestring[20];  

void setup() {
  Serial.begin(115200);

  //ESP8266/FireBase config
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  if(!Firebase.beginStream(FBData,path)){
    Serial.println("Reason: " + FBData.errorReason());
  }

  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  //DHT config
  dht.begin();

  //DS1302 config
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  if (!Rtc.IsDateTimeValid()) 
    {
        Serial.println("RTC lost confidence in the DateTime!");
        Rtc.SetDateTime(compiled);
    }

    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }

    //Servo config
    Roofservo.attach(D5);

    //Led config
    for(int i = 0; i<3; i++){
      pinMode(Led[i],OUTPUT);      
    }
}

void loop() {
  delay(500);

  //Get Hum & Temp
  getDHT11();

  //Get Water Data
  getWaterSensor();

  //Get DateTime
  getDS1302();

  //Get Servo Angle 
  ServoRoof();

  //Control Led 
  ControlLed();

}

void getDHT11(){
  Hum = dht.readHumidity();
  Temp = dht.readTemperature();

  if (isnan(Hum) || isnan(Temp)){
    Serial.println("Error read DHT11");
    Firebase.setString(FBData, path + "/ESP8266/DHT11/error", "Read/Connect");
  }else{
    Firebase.setString(FBData, path + "/ESP8266/DHT11/error", "Non");    
    Firebase.setFloat(FBData, path + "/ESP8266/DHT11/hum", Hum);
    Firebase.setFloat(FBData, path + "/ESP8266/DHT11/temp", Temp);
  }
}

void getWaterSensor(){
  WaterData = analogRead(WATERSENSORPIN);
  Firebase.setInt(FBData, path + "/ESP8266/WATERSENSOR/waterdata", WaterData);
}

void getDS1302(){
  RtcDateTime now = Rtc.GetDateTime();
    if (!now.IsValid())
    {
      Firebase.setString(FBData, path + "/ESP8266/DS1302/error", "Read/Connect/Battery"); 
    }else{
      printDateTime(now);
      Firebase.setString(FBData, path + "/ESP8266/DS1302/error", "Non");    
      Firebase.setString(FBData, path + "/ESP8266/DS1302/date", datestring);
      Firebase.setString(FBData, path + "/ESP8266/DS1302/time", timestring);   
    }
}

void printDateTime(const RtcDateTime& dt)
{
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u"),
            dt.Month(),
            dt.Day(),
            dt.Year());
    snprintf_P(timestring, 
            countof(timestring),
            PSTR("%02u:%02u:%02u"),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
}

void ServoRoof(){
  if(Firebase.getString(FBData, path + "/ESP8266/Servo/trigger")){
    String trig = FBData.stringData();
    if(Firebase.getInt(FBData, path + "/ESP8266/Servo/roof") && trig == "Trig"){  
    int angle = FBData.intData();
    if(angle == 180){   
      for (angle = 0; angle <= 180; angle += 1) {  
        Roofservo.write(angle);
      }
    }else{
      for (angle = 180; angle >= 0; angle -= 1) {  
      Roofservo.write(angle);
        }
      }   
    }
  }
}

void ControlLed(){

  int SLed[] = {0,0,0};
  String refLed[] = {"led1","led2","led3"};

  for(int i = 0; i<3; i++){
    if(Firebase.getInt(FBData, path + "/ESP8266/LED/" + refLed[i])){  
      SLed[i] = FBData.intData();
      digitalWrite(Led[i],SLed[i]);
    }  
  }
}


