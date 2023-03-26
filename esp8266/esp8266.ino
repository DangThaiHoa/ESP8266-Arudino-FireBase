//LIB
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include "FirebaseESP8266.h"
#include <DHT.h>

//ESP8266/FireBase config
#define WIFI_SSID "Home 2.4Ghz"
#define WIFI_PASSWORD "homecafe24"

#define FIREBASE_HOST "homecontrol-60d7d-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "30OHsxJ78Z75ggJgcSI0Zd3COc3E55h6SdiSssuN"

FirebaseData FBData;

String path = "/";
FirebaseJson json;

//DHT config
#define DHTPIN D1     
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);

//Var
float Hum; 
float Temp;


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
}

void loop() {
  delay(500);
  //Get Hum & Temp
  Hum = dht.readHumidity();
  Temp = dht.readTemperature();
  Serial.println(Temp);
  Serial.println(Hum);

  //Update Hum & Temp Data FireBase
  Firebase.setFloat(FBData, path + "/DHT11/hum", Hum);
  Firebase.setFloat(FBData, path + "/DHT11/temp", Temp);
}
