#include <Arduino.h>
#include <stdlib.h>
#include <Adafruit_MAX31865.h>
#include <math.h>
#include <SPI.h>
#include <Wire.h>

#include "env.h"
#include "bosquesFramework.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"

//Pin definitons
#define CS 27     //Violeta
#define SDI 23    //Azul
#define SDO 19    //Verde
#define CLK 18    //Naranja
#define REL 17    //Gris
#define ERR 16    //Marron

//Sensor definitions
double resistance;
float ratio;
uint16_t RTD;
double temperature;
float RREF = 430.0;

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

// WiFi Definitions
String apiUrl = "";
bool offlineMode = true;
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
int setTemp = 102;
int rampCutoff = setTemp - 12;
int rampTreshold = setTemp - 3;
bool firstCicle;

//Sensor Definitions
Adafruit_MAX31865 tempReader = Adafruit_MAX31865(27, 23, 19, 18);
max31865_fault_cycle_t maxFault = MAX31865_FAULT_AUTO;
max31865_numwires_t wires = MAX31865_3WIRE;

class Log{
  public:
    double temperatureLog;
    String timestampLog;
};

Log TempLog;

// void wifiSetup(char* ssid, char* passWifi){
//   int connStatus = 0;
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, passWifi);
//   Serial.println("Conectando a Wifi");
//   delay (500);
//   Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
//   delay (500);
//   for (int i=0; i<10; i++){
//     if (WiFi.status() != WL_CONNECTED) {
//       Serial.print('.');
//       delay (1000);
//     }
//     else{
//       Serial.println("Conectado al Wifi");
//       connStatus = 1;
//       break;
//     }
//   }
//   if (WiFi.status() != WL_CONNECTED){
//     Serial.println("Error al conectarse al WiFi");
//     noWiFi = true;
//   }
//   Serial.println(WiFi.localIP());
// }

void sensorSetup(){
    uint8_t fault = tempReader.readFault();
  if (fault) {
    Serial.print("Fault 0x"); Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      Serial.println("RTD High Threshold");
      error = true; 
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      Serial.println("RTD Low Threshold");
      error = true;  
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      Serial.println("REFIN- > 0.85 x Bias");
      error = true;  
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      Serial.println("REFIN- < 0.85 x Bias - FORCE- open");
      error = true;  
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      Serial.println("RTDIN- < 0.85 x Bias - FORCE- open");
      error = true;  
    }
    if (fault & MAX31865_FAULT_OVUV) {
      Serial.println("Under/Over voltage");
      error = true;  
    }
    tempReader.clearFault();

    while (error = true){
    Serial.println("Error al inciar el sensor, reiniciando");
    for (int i=0; i<9; i++){
      Serial.print(".");
      }
    ESP.restart();
    }
  }
  else{
    Serial.println("Sensor inicializado correctamente");
  }
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
temperature = tempReader.temperature(100,RREF);
Serial.print("Temperatura detectada: "); Serial.println(temperature);
}

void controlTemp(int selected, int tempNow){
  int top = selected;
  int bottom = (selected -1);
  if (tempNow <= bottom){
    digitalWrite(REL, HIGH);
  }
  else if (tempNow >= top){
    digitalWrite(REL, LOW);
  }
}

void firstRamp(int cutoff, int treshold){
  digitalWrite(REL, HIGH);
  delay(100);
  while (firstCicle == true){
    readTemp();
    Serial.println("Temperatura detectada: "); Serial.print(temperature);
    if (temperature > cutoff){
      digitalWrite(REL, LOW);
    }
    if (temperature > treshold){
      firstCicle = false;
      Serial.print(" Saliendo de rampa inicial");
    }
    delay (1000);
  }
}



void subRoutine1(){
  firstRamp(rampCutoff, rampTreshold);
  readTemp();
  controlTemp(setTemp,temperature);
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
  SPI.begin();
  SPI.setClockDivider( SPI_CLOCK_DIV16 );
  SPI.setDataMode( SPI_MODE3 );
  Serial.begin(115200);

  pinMode(REL, OUTPUT);
  pinMode(ERR, OUTPUT);
  
  tempReader.begin(wires);
  sensorSetup();

  noWiFi = wifiSetup(SSID, WIFIPASS);
  offlineMode = modeSetup(apiUrl);

  Serial.printf("Estados - WIFI: %s - ONLINE: %s",noWiFi,offlineMode);

  if (!noWiFi){
    Serial.println("Antes de firebaseSetup");
    firebaseSetup();
  
    if(!offlineMode){
      Serial.println("Dnetro del config de firebase");
      Firebase.begin(&config, &auth);
      config.fcs.download_buffer_size = 2048;
      Firebase.reconnectWiFi(true);
    }
  }
  
  Serial.printf("Firebase status: %s - Task: %s",Firebase.ready(), !taskCompleted);

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