#include <Arduino.h>
#include <stdlib.h>
#include <DHT.h>
#include <math.h>
#include <env.h>
#include <bosquesFramework.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <floatToString.h>
#include "addons/TokenHelper.h"
#include "firmware.h"

// Pin definitons
#define SEN 18
#define VEN 2
#define HEL 32
#define REL 25

// Sensor definitions
#define DHTTYPE DHT22
DHT sensor(SEN, DHTTYPE);

// WiFi Definitions
const String apiUrl = API_URL;
const String getTempUrl = apiUrl + "/v1/heladera/gettemp";
const String updateTempUrl = apiUrl + "/v1/heladera/updatetemp";
const String statusUrl = apiUrl + "/status";
static String taskParams[3] = {statusUrl, getTempUrl, updateTempUrl};

// Current firmware version
#define CURRENT_FIRMWARE_VERSION "3.0.2"

// Flags
bool taskCompleted = false;
bool error = false;
bool noWiFi = false;
bool offlineMode = false;
bool heladeraOn = false;
bool lampOn = false;
bool tempBlocked = false;

// Temperature Settings
int defaultTemp = 24;
float setTemp = defaultTemp;

float minTempHela = setTemp + 0.3;
float maxTempHela = setTemp + 0.5;
float minTempLamp = setTemp - 0.3;
float maxTempLamp = setTemp - 0.5;

float temperature;
float temp;

void sensorSetup()
{
  sensor.begin();
}

// Lee temp del sensor/sensores
void readTemp()
{
  temp = sensor.readTemperature();
  temperature = temp;
  Serial.printf("INFO - Temperatura detectada sensor: %f\r\n", temperature);
}

// Rutina de control
void controlTemp(float minHela, float maxHela, float minLamp, float maxLamp, float tempNow)
{
  if (isnan(tempNow))
  {
    Serial.println("\t Temperatura leida invalida Heladera");
  }
  else
  {
    Serial.printf("CONTROL - Temperatura de trabajo Heladera: %f \n", setTemp);
    if (tempNow >= minHela)
    {
      if (tempNow >= maxHela)
      {
        digitalWrite(HEL, LOW);
        digitalWrite(VEN, LOW);
        digitalWrite(REL, HIGH);
        heladeraOn = true;
        Serial.println("Modo: Encender heladera - Heladera Simple\n");
      }
      else
      {
        if (tempNow <= minHela && heladeraOn)
        {
          digitalWrite(HEL, HIGH);
          digitalWrite(VEN, HIGH);
          heladeraOn = false;
          Serial.println("Modo apagar heladera - Heladera Simple\n");
        }
      }
    }
    else if (tempNow <= minLamp)
    {
      if (tempNow <= maxLamp)
      {
        digitalWrite(REL, LOW);
        digitalWrite(VEN, LOW);
        digitalWrite(HEL, HIGH);
        lampOn = true;
        Serial.println("Modo encender lampara - Heladera Simple\n");
      }
      else
      {
        if (tempNow >= minLamp && lampOn)
        {
          digitalWrite(REL, HIGH);
          digitalWrite(VEN, HIGH);
          lampOn = false;
          Serial.println("Modo apagar lampara Heladera Simple\n");
        }
      }
    }

    else if (!lampOn || !heladeraOn)
    {
      digitalWrite(REL, HIGH);
      digitalWrite(VEN, HIGH);
      digitalWrite(HEL, HIGH);
      Serial.println("Modo apagar todo - Heladera Simple\n");
    }
  }
}

// Actualiza valores de margenes
void tempsUpdate(float tempNow)
{
  minTempHela = tempNow + 0.2;
  maxTempHela = tempNow + 0.5;
  minTempLamp = tempNow - 0.2;
  maxTempLamp = tempNow - 0.5;
}

void subRoutineControl()
{
  readTemp();
  controlTemp(minTempHela, maxTempHela, minTempLamp, maxTempLamp, temperature);
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
        // if there is a new update, download it
        if (checkForUpdate(CURRENT_FIRMWARE_VERSION))
        {
          downloadAndUpdateFirmware();
        }
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(REL, OUTPUT);
  pinMode(VEN, OUTPUT);
  pinMode(HEL, OUTPUT);
  digitalWrite(REL, HIGH);
  digitalWrite(VEN, HIGH);
  digitalWrite(HEL, HIGH);

  Serial.printf("Firmware v%s\r\n", CURRENT_FIRMWARE_VERSION);

  sensorSetup();
  noWiFi = wifiSetup(SSID, WIFIPASS);
  modeSetup(apiUrl);
  firebaseSetup();

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