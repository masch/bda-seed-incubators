#include <Arduino.h>
#include <stdlib.h>
#include <Adafruit_MAX31865.h>
#include <math.h>
#include <SPI.h>
#include <Wire.h>
#include <env.h>
#include <bosquesFramework.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Firebase_ESP_Client.h>
#include <floatToString.h>
#include "addons/TokenHelper.h"

// Pin definitons
#define CS 27  // Violeta
#define SDI 23 // Azul
#define SDO 19 // Verde
#define CLK 18 // Naranja
#define REL 17 // Gris
#define ERR 16 // Marron

// Sensor definitions
double resistance;
float ratio;
uint16_t RTD;
double temperature;
float RREF = 430.0;

// WiFi Definitions
String apiUrl = "";
String getTempUrl = apiUrl + "/v1/horno/gettemp";
String updateTempUrl = apiUrl + "/v1/horno/updatetemp";
const String statusUrl = apiUrl + "/status";
static String taskParams[3] = {statusUrl, getTempUrl, updateTempUrl};

// Firebase Definitions
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Flags
bool taskCompleted = false;
bool error = false;
bool noWiFi = false;
bool firstCicle = true;
bool offlineMode = true;
bool lowTempMode = false;

// Tmperature Settings
int defaultTemp = 85;
int setTemp = defaultTemp;
int rampCutoff = setTemp - 9;
int rampTreshold = setTemp - 3;

// Sensor Definitions
Adafruit_MAX31865 tempReader = Adafruit_MAX31865(27, 23, 19, 18);
max31865_fault_cycle_t maxFault = MAX31865_FAULT_AUTO;
max31865_numwires_t wires = MAX31865_3WIRE;

void sensorSetup()
{
  uint8_t fault = tempReader.readFault();
  if (fault)
  {
    Serial.print("Fault 0x");
    Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH)
    {
      Serial.println("RTD High Threshold");
      error = true;
    }
    if (fault & MAX31865_FAULT_LOWTHRESH)
    {
      Serial.println("RTD Low Threshold");
      error = true;
    }
    if (fault & MAX31865_FAULT_REFINLOW)
    {
      Serial.println("REFIN- > 0.85 x Bias");
      error = true;
    }
    if (fault & MAX31865_FAULT_REFINHIGH)
    {
      Serial.println("REFIN- < 0.85 x Bias - FORCE- open");
      error = true;
    }
    if (fault & MAX31865_FAULT_RTDINLOW)
    {
      Serial.println("RTDIN- < 0.85 x Bias - FORCE- open");
      error = true;
    }
    if (fault & MAX31865_FAULT_OVUV)
    {
      Serial.println("Under/Over voltage");
      error = true;
    }
    tempReader.clearFault();

    while (error == true)
    {
      Serial.println("Error al inciar el sensor, reiniciando");
      for (int i = 0; i < 9; i++)
      {
        Serial.print(".");
        delay(700);
      }
      ESP.restart();
    }
  }
  else
  {
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

void firebaseSetup()
{
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;
}

void readTemp()
{
  temperature = tempReader.temperature(100, RREF);
  Serial.printf("INFO - Temperatura detectada sensor: %f\n", temperature);
}

void controlTemp(int selected, int tempNow)
{
  int top = selected;
  int bottom = (selected - 1);
  if (tempNow <= bottom)
  {
    digitalWrite(REL, HIGH);
  }
  else if (tempNow >= top)
  {
    digitalWrite(REL, LOW);
  }
}

void firstRamp(int cutoff, int treshold)
{
  digitalWrite(REL, HIGH);
  delay(100);
  while (firstCicle == true)
  {
    readTemp();
    Serial.println("Temperatura detectada: ");
    Serial.print(temperature);
    if (temperature > cutoff)
    {
      digitalWrite(REL, LOW);
      delay(10000);
    }
    if (temperature > treshold)
    {
      firstCicle = false;
      Serial.print(" Saliendo de rampa inicial");
      break;
    }
    else if (REL == LOW && firstCicle == true)
    {
      digitalWrite(REL, HIGH);
    }
    delay(1000);
  }
}

void tempsUpdate(float tempNow)
{
  rampCutoff = tempNow - 9;
  rampTreshold = tempNow - 3;
}

void subRoutineControl()
{

  if (setTemp < 51)
  {
    lowTempMode = true;
  }
  else
  {
    lowTempMode = false;
  }

  if (lowTempMode == false)
  {
    if (setTemp < 150)
    {
      firstRamp(rampCutoff, rampTreshold);
    }
  }

  readTemp();
  controlTemp(setTemp, temperature);
  delay(1000);
}

void subRoutineInternet(void *params)
{
  String *urls = (String *)params;
  while (true)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      offlineMode = true;
      Serial.println("Sin WiFi, reconectando. ");
      vTaskDelay(5000);
      wifiSetup(SSID, WIFIPASS);
    }
    else
    {
      offlineMode = modeSetup(urls[0]);
      if (!offlineMode)
      {
        vTaskDelay(5000);
        setTemp = getServerTemp(urls[1], setTemp);
        tempsUpdate(setTemp);
        updateServerTemp(urls[2], temperature);
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setDataMode(SPI_MODE3);

  pinMode(REL, OUTPUT);
  pinMode(ERR, OUTPUT);

  Serial.println("Horno de Semillas");
  Serial.printf("Firmware v%s\n", FIRMWARE_VERSION);
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  tempReader.begin(wires);
  sensorSetup();
  noWiFi = wifiSetup(SSID, WIFIPASS);
  modeSetup(apiUrl);

  if (!noWiFi)
  {
    firebaseSetup();

    if (!offlineMode)
    {
      Firebase.begin(&config, &auth);
      config.fcs.download_buffer_size = 2048;
      Firebase.reconnectWiFi(true);
    }
  }

  if (Firebase.ready() && !taskCompleted)
  {
    taskCompleted = true;
    Serial.println("\nChecking for new firmware update available...\n");

    if (!Firebase.Storage.downloadOTA(
            &fbdo, STORAGE_BUCKET_ID,
            FIRMWARE_PATH,
            firmwareDownload))
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

  xTaskCreatePinnedToCore(
      subRoutineInternet,
      "Internet Stack",
      20000,
      (void *)taskParams,
      0,
      NULL,
      1);
}

void loop()
{
  while (error != true)
  {
    if (offlineMode)
    {
      Serial.println("## - OFFLINE - ##");
    }
    else
    {
      Serial.println("## - ONLINE - ##");
    }
    subRoutineControl();
  }
  Serial.println(" ## -  ERROR, REINICIANDO ESP - ##");
  ESP.restart();
}