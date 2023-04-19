//LIB
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "FirebaseESP8266.h"
#include <DHT.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>
#include <Servo.h>

//FactoryReset Config 
int buttonPin = 0;
int buttonState = 1;     // current state of the button
int lastButtonState = 0; // previous state of the button
int startPressed = 0;    // the moment the button was pressed
int endPressed = 0;      // the moment the button was released
int holdTime = 0;        // how long the button was hold
int idleTime = 0;  

//Config ESP8266 AP
#define APSSID "ESP8266_Config"
#define APPSK "esp8266config"

ESP8266WebServer server(80);

//WebServerHTML
const char main_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Thiết lập ESP8266</title>
</head>

<body>
    <h3 style="text-align: center;">Hãy Nhập Tên WiFi và Mật Khẩu Để Thiết Lập Mạch ESP8266</h3>
    <form action="/ConfigESP" style="text-align: center;">
        <h3>Tên WIFI:</h3><input name="ssid" type="text" /><br>
        <h3>Mật Khẩu:</h3><input name="pass" type="text" /><br><br>
        <input type="submit" value="Xác Nhận" />
    </form>
</body>

</html>
)=====";
String html = main_page;

const char success_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Thiết lập ESP8266</title>
</head>

<body>
    <h3 style="text-align: center;">Đã thiết lập thiết bị ESP82666 Nếu Led nháy 4 lần là thiết lập thành công, Hãy quay lại ứng dụng và kiểm tra, nếu chưa được hãy thiết lập lại</h3>
</body>

</html>
)=====";
String Shtml = success_page;

//ValConfig
String UID;
String WIFI_SSID;
String WIFI_PASSWORD;
String GUID;
String GWIFI_SSID;
String GWIFI_PASSWORD;

//ESP8266/FireBase config
#define FIREBASE_HOST "homecontrol-526d0-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "DqSPT3XALNg7QRlWd0AYzbvcYp0O2799ZaFRUL4e"

FirebaseData FBData;

String path = "/";
FirebaseJson json;

//DHT config
#define DHTPIN 5 //D1    
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);
int pos = 0, num = 0;

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

//RGB
String gCurrent;

//Var
float Hum; 
float Temp;

int WaterData;

char datestring[20];
char timestring[20];

//FunctionConfigESP8266

void Connecting() {
  UID = server.arg("UID");
  String webServer = main_page;
  server.send(200, "text/html", webServer);
}

void ConfigESP8266(){
  WIFI_SSID = server.arg("ssid");
  WIFI_PASSWORD = server.arg("pass");
  if (WIFI_SSID.length() > 0 && WIFI_PASSWORD.length() > 0) {
          Serial.println("");
        Serial.println("clearing eeprom");
        for (int i = 0; i < 255; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < WIFI_SSID.length(); ++i)
        {
          EEPROM.write(i, WIFI_SSID[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < WIFI_PASSWORD.length(); ++i)
        {
          EEPROM.write(32 + i, WIFI_PASSWORD[i]);
        }
        for (int i = 0; i < UID.length(); ++i)
        {
          EEPROM.write(96 + i, UID[i]);
        }
        EEPROM.commit();
  String SwebServer = Shtml;
  server.send(200, "text/html",SwebServer);
  delay(10000);
  ESP.reset();
  }
}

bool TestWiFiConnect(void){
  int Count = 0 ;
  while(Count < 20 ){
    if(WiFi.status() == WL_CONNECTED){
      return true;
    }
    delay(500);
    Serial.print("*");
    Count++;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  WiFi.disconnect();
  EEPROM.begin(255);

  // GetSSID & PASSWORD
  for (int i = 0; i < 32; ++i){
    GWIFI_SSID += char(EEPROM.read(i));
  }
  for (int i = 32; i < 96; ++i)
  {
    GWIFI_PASSWORD += char(EEPROM.read(i));
  }
  for (int i = 96; i < 255; ++i)
  {
    GUID += char(EEPROM.read(i));
  }

  // Test WiFi Connect
  // WiFi.begin(GWIFI_SSID, GWIFI_PASSWORD);
  WiFi.begin("Home 2.4Ghz", "homecafe24");
  if(TestWiFiConnect()){
    // ESP8266/FireBase Config
    Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);
    if(!Firebase.beginStream(FBData,path)){
      Serial.println("Reason: " + FBData.errorReason());
    }
    Serial.println("");
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    //01
    Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-01/uid", GUID);
    Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-01/role", "Owner"); 
    //02
    Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-02/uid", "null");
    Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-02/role", "null");
    Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-02/email", "null");
    //03
    Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-03/uid", "null");
    Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-03/role", "null");
    Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-03/email", "null");
    
    // DHT config
    dht.begin();

    // DS1302 config
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

      //FactoryReset Config
      pinMode(buttonPin, INPUT_PULLUP);
      pinMode(LED_BUILTIN, OUTPUT);

      for(int j =0; j<3; j++){
        delay(500);  
        digitalWrite(LED_BUILTIN, HIGH);  
        delay(500);                
        digitalWrite(LED_BUILTIN, LOW);
      }
      return;
  }else{
    //StartAPMode
    Serial.println();
    Serial.print("Configuring access point...");
    WiFi.softAP(APSSID, APPSK);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    server.on("/", Connecting);
    server.on("/ConfigESP", ConfigESP8266);
    server.begin();
    Serial.println("HTTP server started");
  }
}

void loop() {
  delay(1000);

  //FactoryReset
  buttonState = digitalRead(buttonPin); 
  if (buttonState != lastButtonState) { 
    updateState();
  }
  lastButtonState = buttonState;

  if(!TestWiFiConnect()){
    //getEventServer
    server.handleClient();
  }else{

    //CheckConnect
    if(Firebase.getString(FBData, path + "/HomeControl/ESP8266/Connect")){
    String getCheck = FBData.stringData(); 
      if(getCheck == "null"){
        Firebase.setString(FBData, path + "/HomeControl/ESP8266/Connect", "isConnect");       
      }
    }

    //Get Servo Angle 
    ServoRoof();

    //Control Led 
    ControlLed();

    //FactoryReset
    FacReset();

    //Get Hum & Temp
    getDHT11();

    //Get Water Data
    getWaterSensor();

    //Get DateTime
    getDS1302();

    //Get RGB Code
    RGB();
  }

}

//FactoryReset
void updateState() {
  // the button has been just pressed
  if (buttonState == HIGH) {
      startPressed = millis();
  } else {
      endPressed = millis();
      holdTime = endPressed - startPressed;
      if (holdTime >= 3000) {
        for(int j =0; j<5; j++){
          delay(500);  
          digitalWrite(LED_BUILTIN, HIGH);  
          delay(500);                
          digitalWrite(LED_BUILTIN, LOW);
        }
        for(int i = 0; i<255; i++){
        EEPROM.write(i,0);
        delay(20);
        Serial.println("Deleting");
        }
        EEPROM.commit();
        Serial.println("Deleted EEPROM Complete");
        UID = "";
        WIFI_SSID = "";
        WIFI_PASSWORD = "";
        GUID = "";
        GWIFI_SSID = "";
        GWIFI_PASSWORD = "";
        }
        delay(1000);
        ESP.reset();   
  }
}

void FacReset(){
  if(Firebase.getInt(FBData, path + "/HomeControl/ESP8266/Reset")){
    int getActive = FBData.intData();
    if(getActive == 1){
      for(int j =0; j<5; j++){
        delay(500);  
        digitalWrite(LED_BUILTIN, HIGH);  
        delay(500);                
        digitalWrite(LED_BUILTIN, LOW);
      }
      for(int i = 0; i<255; i++){
        EEPROM.write(i,0);
        delay(20);
        Serial.println("Deleting");
      }
      EEPROM.commit();
      Serial.println("Deleted EEPROM Complete");
      UID = "";
      WIFI_SSID = "";
      WIFI_PASSWORD = "";
      GUID = "";
      GWIFI_SSID = "";
      GWIFI_PASSWORD = "";
      //01
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-01/uid", "null");
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-01/role", "null");
      //02
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-02/uid", "null");
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-02/role", "null");
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-02/email", "null");
      //03
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-03/uid", "null");
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-03/role", "null");
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/Users/UID-03/email", "null");
      delay(1000);
      ESP.reset();
    }
  }
  
}

void getDHT11(){
  Hum = dht.readHumidity();
  Temp = dht.readTemperature();

  if (isnan(Hum) || isnan(Temp)){
    Firebase.setString(FBData, path + "/HomeControl/ESP8266/DATA/DHT11/error", "Read/Connect");
  }else{
    Firebase.setString(FBData, path + "/HomeControl/ESP8266/DATA/DHT11/error", "Non");    
    Firebase.setFloat(FBData, path + "/HomeControl/ESP8266/DATA/DHT11/hum", Hum);
    Firebase.setFloat(FBData, path + "/HomeControl/ESP8266/DATA/DHT11/temp", Temp);
    if(pos == 10 && num == 10){
      pos = 0;
      num = 0;   
    }else{
      delay(1000);
      //Set Pos/Data Temp
      Firebase.setInt(FBData, path + "/HomeControl/ESP8266/ChartData/Temperature/key" + num  + "/pos", pos); 
      Firebase.setFloat(FBData, path + "/HomeControl/ESP8266/ChartData/Temperature/key" + num  + "/data", Temp);

      //Set Pos/Data Hum
      Firebase.setInt(FBData, path + "/HomeControl/ESP8266/ChartData/Humanity/key" + num  + "/pos", pos); 
      Firebase.setFloat(FBData, path + "/HomeControl/ESP8266/ChartData/Humanity/key" + num  + "/data", Hum);

      num += 1;
      pos += 1; 
    }
  }
}

void getWaterSensor(){
  WaterData = analogRead(WATERSENSORPIN);
  Firebase.setInt(FBData, path + "/HomeControl/ESP8266/DATA/WATERSENSOR/waterdata", WaterData);
}

void getDS1302(){
  RtcDateTime now = Rtc.GetDateTime();
    if (!now.IsValid())
    {
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/DATA/DS1302/error", "Read/Connect/Battery"); 
    }else{
      printDateTime(now);
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/DATA/DS1302/error", "Non");    
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/DATA/DS1302/date", datestring);
      Firebase.setString(FBData, path + "/HomeControl/ESP8266/DATA/DS1302/time", timestring);   
    }
}

void printDateTime(const RtcDateTime& dt){
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
  if(Firebase.getString(FBData, path + "/HomeControl/ESP8266/DATA/Servo/trigger")){
    String trig = FBData.stringData();
    if(Firebase.getInt(FBData, path + "/HomeControl/ESP8266/DATA/Servo/roof")){  
    int angle = FBData.intData();
    Roofservo.write(angle);
  }
}
}

void ControlLed(){
    if(Firebase.getInt(FBData, path + "/HomeControl/ESP8266/DATA/LED/led1")){  
      int sLed1 = FBData.intData();
      digitalWrite(Led[0],sLed1);
    }  
    if(Firebase.getInt(FBData, path + "/HomeControl/ESP8266/DATA/LED/led2")){  
      int sLed2 = FBData.intData();
      digitalWrite(Led[1],sLed2);
    }  
    if(Firebase.getInt(FBData, path + "/HomeControl/ESP8266/DATA/LED/led3")){  
      int sLed3 = FBData.intData();
      digitalWrite(Led[2],sLed3);
    }  
}

void RGB(){

  int red;
  int green;
  int blue;
  int Trig;

  if(Firebase.getInt(FBData, path + "/HomeControl/ESP8266/DATA/RGB/red")){  
    red = FBData.intData();
  }  
  if(Firebase.getInt(FBData, path + "/HomeControl/ESP8266/DATA/RGB/green")){  
    green = FBData.intData();
  }  
  if(Firebase.getInt(FBData, path + "/HomeControl/ESP8266/DATA/RGB/blue")){  
    blue = FBData.intData();
  }  
  if(Firebase.getInt(FBData, path + "/HomeControl/ESP8266/DATA/RGB/status")){  
    Trig = FBData.intData();
  } 

  String status;
  if(Trig == 1){
    status = "1";
  }else{
    status = "0";
  } 

  String gRed = String(red); 
  String gGreen = String(green); 
  String gBlue = String(blue); 
  String gTrig = String(Trig); 
  String p = ",";

  gRed.concat(p);
  gRed.concat(gGreen);
  gRed.concat(p);
  gRed.concat(gBlue);
  gRed.concat(p);
  gRed.concat(status);

  if(gRed != gCurrent){
    Serial.println(gRed);
  }
  gCurrent = gRed;
}


