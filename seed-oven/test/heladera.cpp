#include <Arduino.h>
#include <stdlib.h>
#include <DHT.h>
#include <math.h>

#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"

//Pin definitons
#define S1 15
#define S2 16
#define VEN 5
#define HEL 18
#define REL 17

//Sensor definitions
#define DHTTYPE DHT22
DHT sensor1(S1, DHTTYPE);
DHT sensor2(S2, DHTTYPE);

//WiFi Definitions
char* ssid = "";
char* wifiPass = "";

//Firebase Definitions
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
#define API_KEY ""
#define USER_EMAIL ""
#define USER_PASSWORD ""
#define STORAGE_BUCKET_ID ""
#define FIRMWARE_PATH ""

bool taskCompleted = false;
bool error = false;
bool noWiFi = false;

//Timestamp Definitions
String formattedDate;
String dateTime;
String hourTime;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//Tmperature Settings
int minTemp = 22;
int maxTemp = 24;
float temperature;
float temp1, temp2;

class Log{
  public:
    double temperatureLog;
    String timestampLog;
};

Log TempLog;

void wifiSetup(char* ssid, char* passWifi){
  int connStatus = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passWifi);
  Serial.println("Conectando a Wifi");
  delay (500);
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  delay (500);
  for (int i=0; i<10; i++){
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay (1000);
    }
    else{
      Serial.println("Conectado al Wifi");
      connStatus = 1;
      break;
    }
  }
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Error al conectarse al WiFi");
    noWiFi = true;
  }
  Serial.println(WiFi.localIP());
}

void sensorSetup(){
  sensor1.begin();
  sensor2.begin();
}

void firmwareDownload(FCS_DownloadStatusInfo info)
{
    if (info.status == fb_esp_fcs_download_status_init)
    {
        Serial.printf("New update found\n");
        Serial.printf("Downloading firmware %s (%d bytes)\n", info.remoteFileName.c_str(), info.fileSize);
    }
    else if (info.status == fb_esp_fcs_download_status_download)
    {
        Serial.printf("Downloaded %d%s\n", (int)info.progress, "%");
    }
    else if (info.status == fb_esp_fcs_download_status_complete)
    {
        Serial.println("Donwload firmware completed.");
        Serial.println();
    }
    else if (info.status == fb_esp_fcs_download_status_error)
    {
        Serial.printf("New firmware update not available or download failed, %s\n", info.errorMsg.c_str());
    }
}

void firebaseSetup(){
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;
}

void readTemp(){
  temp1 = sensor1.readTemperature();
  temp2 = sensor2.readTemperature();
  temperature = ((temp1 + temp2)/2);
  Serial.print("Temperatura detectada: "); Serial.println(temperature);
}

void controlTemp(int min, int max, int tempNow){
  if (isnan(tempNow)){
    Serial.println("Temperatura leida invalida");
  }
  else{
  if (tempNow < min){
    digitalWrite(REL, LOW);
    digitalWrite(VEN, LOW);
  }
  if (tempNow >= min && tempNow <= max){
    digitalWrite(REL, HIGH);
    digitalWrite(VEN, HIGH);
    digitalWrite(HEL, HIGH);
  }
  if (tempNow > max){
    digitalWrite(HEL, LOW);
  }
  }
}


void subRoutine1(){
  readTemp();
  controlTemp(minTemp,maxTemp,temperature);
  delay(1000);
}

void subRoutine2(void* pvParameters){
while(1){
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }

  formattedDate = timeClient.getFormattedTime();


  int splitT = formattedDate.indexOf("T");
  dateTime = formattedDate.substring(0, splitT);
  hourTime = formattedDate.substring(splitT+1, formattedDate.length()-1);
  
  TempLog.timestampLog = hourTime;
  TempLog.temperatureLog = temperature;
  Serial.print("Timestamp"); Serial.print(TempLog.timestampLog);
  delay (700);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(REL, OUTPUT);
  pinMode(VEN, OUTPUT);
  pinMode(HEL, OUTPUT);
  digitalWrite(REL, HIGH);
  digitalWrite(VEN, HIGH);
  digitalWrite(HEL, HIGH);
  
  sensorSetup();
  wifiSetup(ssid, wifiPass);
  if (noWiFi == false){
    digitalWrite(ERR, HIGH);
  }
  
  firebaseSetup();
  Firebase.begin(&config, &auth);
  config.fcs.download_buffer_size = 2048;
  Firebase.reconnectWiFi(true);

  timeClient.begin();
  timeClient.setTimeOffset(-10800);

  if (Firebase.ready() && !taskCompleted)
  {
    taskCompleted = true;
    Serial.println("\nChecking for new firmware update available...\n");

    if (!Firebase.Storage.downloadOTA(
            &fbdo, STORAGE_BUCKET_ID,
            FIRMWARE_PATH,
            firmwareDownload
            ))
    {
      Serial.println(fbdo.errorReason());
    }
    else
    {
      Serial.printf("Delete file... %s\n", Firebase.Storage.deleteFile(&fbdo, STORAGE_BUCKET_ID, FIRMWARE_PATH) ? "ok" : fbdo.errorReason().c_str());
      Serial.println("Restarting...\n\n");
      delay(2000);
      ESP.restart();
    }
  }

   xTaskCreatePinnedToCore (
    subRoutine2,     
    "Logging",   
    20000,      
    NULL,      
    0,         
    NULL,      
    1          
  );
}

void loop(){
  while (error != true){
    subRoutine1();
  }
}