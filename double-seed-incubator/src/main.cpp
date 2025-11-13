#include <Arduino.h>
#include <stdlib.h>
#include <DHT.h>
#include <math.h>
#include <bda/env.h>
#include <bda/firmware.h>
#include <bda/network.h>

#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Firebase_ESP_Client.h>
#include <floatToString.h>
#include "addons/TokenHelper.h"

// Pin definitons
#define S1 18
#define S2 5
#define VEN1 2
#define HEL1 32
#define REL1 25
#define VEN2 4
#define HEL2 33
#define REL2 26

// Sensor definitions
#define DHTTYPE DHT22
DHT sensor1(S1, DHTTYPE);
const char *SENOR_1_NAME = "S1";
DHT sensor2(S2, DHTTYPE);
const char *SENOR_2_NAME = "S2";

// WiFi Definitions
const String bdaApiURL = BDA_API_URL;
String bdaStatusApiURL = bdaApiURL + "/status";
String getTemp1 = bdaApiURL + "/v1/heladera-doble-1/gettemp";
String updateTemp1 = bdaApiURL + "/v1/heladera-doble-1/updatetemp";
String getTemp2 = bdaApiURL + "/v1/heladera-doble-2/gettemp";
String updateTemp2 = bdaApiURL + "/v1/heladera-doble-2/updatetemp";

unsigned long lastMillis;

// Firebase Definitions
// FirebaseData fbdo;
// FirebaseAuth auth;
// FirebaseConfig config;

// Flags
bool taskCompleted = false;
bool error = false;
bool noWiFi = false;
bool offlineMode = true;
bool heladeraOn1, heladeraOn2 = false;
bool lampOn1, lampOn2 = false;

// Timestamp Definitions
String formattedDate;
String dateTime;
String hourTime;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Tmperature Settings
int defaultTemp = 24;
float setTemp1, setTemp2 = defaultTemp;

float minTempHela1 = setTemp1 + 0.3;
float maxTempHela1 = setTemp1 + 0.5;
float minTempLamp1 = setTemp1 - 0.3;
float maxTempLamp1 = setTemp1 - 0.5;

float minTempHela2 = setTemp2 + 0.3;
float maxTempHela2 = setTemp2 + 0.5;
float minTempLamp2 = setTemp2 - 0.3;
float maxTempLamp2 = setTemp2 - 0.5;

float temperature1, temperature2;
float temp1, temp2;

class Log
{
public:
  double temperatureLog;
  String timestampLog;
};

Log TempLog1;
Log TempLog2;

/*void wifiSetup(const char *ssid, const char *passWifi)
{
  int connStatus = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passWifi);
  Serial.println("Conectando a Wifi");
  delay(500);
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  delay(500);
  for (int i = 0; i < 10; i++)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.print('.');
      delay(1000);
    }
    else
    {
      Serial.println("Conectado al Wifi");
      connStatus = 1;
      break;
    }
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Error al conectarse al WiFi");
    noWiFi = true;
  }
  Serial.println(WiFi.localIP());
}*/

void sensorSetup()
{
  sensor1.begin();
  sensor2.begin();
}

/*
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
*/
// void firebaseSetup()
// {
//   config.api_key = API_KEY;
//   auth.user.email = USER_EMAIL;
//   auth.user.password = USER_PASSWORD;
//   config.token_status_callback = tokenStatusCallback;
// }

void readTemp1()
{
  temperature1 = NAN; // Start with an invalid value
  for (int i = 0; i < 2; i++)
  { // Try reading 2 times
    temp1 = sensor1.readTemperature();
    if (!isnan(temp1))
    {
      temperature1 = temp1;
      Serial.printf("INFO - Temperatura detectada sensor: %f\r\n", temperature1);
      return;
    }
    delay(250); // Wait before retrying
  }
  Serial.println("Error: Failed to read temperature from sensor 1");
}

void readTemp2()
{
  temperature2 = NAN; // Start with an invalid value
  for (int i = 0; i < 2; i++)
  { // Try reading 2 times
    temp2 = sensor2.readTemperature();
    if (!isnan(temp2))
    {
      temperature2 = temp2;
      Serial.printf("INFO - Temperatura detectada sensor: %f\r\n", temperature2);
      return;
    }
    delay(250); // Wait before retrying
  }
  Serial.println("Error: Failed to read temperature from sensor 2");
}

void controlTemp1(float minHela, float maxHela, float minLamp, float maxLamp, float tempNow)
{
  if (isnan(tempNow))
  {
    Serial.println("Temperatura leida invalida Heladera 1");
  }
  else
  {
    Serial.printf("CONTROL - Temperatura de trabajo Heladera Doble 1: %f", setTemp1);
    if (tempNow >= minHela)
    {
      if (tempNow >= maxHela)
      {
        digitalWrite(HEL1, LOW);
        digitalWrite(VEN1, LOW);
        digitalWrite(REL1, HIGH);
        heladeraOn1 = true;
        Serial.println("Entra en Modo encender heladera - Heladera 1");
      }
      else
      {
        if (tempNow <= minHela && heladeraOn1)
        {
          digitalWrite(HEL1, HIGH);
          digitalWrite(VEN1, HIGH);
          heladeraOn1 = false;
          Serial.println("Entra en Modo apagar heladera - Heladera 1");
        }
      }
    }

    else if (tempNow <= minLamp)
    {
      if (tempNow <= maxLamp)
      {
        digitalWrite(REL1, LOW);
        digitalWrite(VEN1, LOW);
        digitalWrite(HEL1, HIGH);
        lampOn1 = true;
        Serial.println("Entra en Modo encender lampara - Heladera 1");
      }
      else
      {
        if (tempNow >= minLamp && lampOn1)
        {
          digitalWrite(REL1, HIGH);
          digitalWrite(VEN1, HIGH);
          lampOn1 = false;
          Serial.println("Entra en Modo apagar lampara Heladera 1");
        }
      }
    }

    else if (!lampOn1 || !heladeraOn1)
    {
      digitalWrite(REL1, HIGH);
      digitalWrite(VEN1, HIGH);
      digitalWrite(HEL1, HIGH);
      Serial.println("Entra en Modo apagar todo - Heladera 1");
    }
  }
}

void controlTemp2(float minHela, float maxHela, float minLamp, float maxLamp, float tempNow)
{
  Serial.println("ControlTemp2");

  if (isnan(tempNow))
  {
    Serial.println("Temperatura leida invalida Heladera 2");
  }
  else
  {
    Serial.printf("CONTROL - Temperatura de trabajo Heladera Doble 2: %f", setTemp2);
    if (tempNow >= minHela)
    {
      if (tempNow >= maxHela)
      {
        digitalWrite(HEL2, LOW);
        digitalWrite(VEN2, LOW);
        digitalWrite(REL2, HIGH);
        heladeraOn2 = true;
        Serial.println("Entra en Modo encender heladera - Heladera 2");
      }
      else
      {
        if (tempNow <= minHela && heladeraOn2)
        {
          digitalWrite(HEL2, HIGH);
          digitalWrite(VEN2, HIGH);
          heladeraOn2 = false;
          Serial.println("Entra en Modo apagar heladera - Heladera 2");
        }
      }
    }

    else if (tempNow <= minLamp)
    {
      if (tempNow <= maxLamp)
      {
        digitalWrite(REL2, LOW);
        digitalWrite(VEN2, LOW);
        digitalWrite(HEL2, HIGH);
        lampOn2 = true;
        Serial.println("Entra en Modo encender lampara - Heladera 2");
      }
      else
      {
        if (tempNow >= minLamp && lampOn2)
        {
          digitalWrite(REL2, HIGH);
          digitalWrite(VEN2, HIGH);
          lampOn2 = false;
          Serial.println("Entra en Modo apagar lampara - Heladera 2");
        }
      }
    }

    else if (!lampOn2 || !heladeraOn2)
    {
      digitalWrite(REL2, HIGH);
      digitalWrite(VEN2, HIGH);
      digitalWrite(HEL2, HIGH);
      Serial.println("Entra en Modo apagar todo - Heladera 2");
    }
  }
}

/*
float getServerTemp(String url, float setTemp)
{
  float serverTemp = setTemp;

  http.begin(url.c_str());
  int responseCode = http.GET();
  String responseTemp = http.getString();

  if (responseCode == 200)
  {
    serverTemp = responseTemp.toFloat();
  }
  else
  {
    Serial.println("Error: ");
    Serial.println(responseCode);
  }
  Serial.printf("CONTROL - Temperatura leida del Servidor: %f", serverTemp);
  http.end();
  return serverTemp;
}

void updateServerTemp(String url, float newtemp)
{
  char buffer[6];
  String sentTemp = floatToString(newtemp, buffer, 6, 2);

  http.begin(url.c_str());
  http.addHeader("temp", sentTemp);
  int responseCode = http.POST("");

  if (responseCode == 200)
  {
    Serial.println("Temperatura enviada correctamente a servidor");
  }
  else
  {
    Serial.println("Error: ");
    Serial.print(responseCode);
  }
  http.end();
}

void modeSetup(String url)
{
  http.begin(url.c_str());
  int response = http.GET();
  Serial.print(response);
  if (noWiFi != true && response == 200)
  {
    offlineMode = false;
    Serial.println("Inicalizado en modo ONLINE");
  }
  else
  {
    Serial.println("Inicializado en modo OFFLINE");
  }
  http.end();
}
*/

void tempsUpdate(float tempNow1, float tempNow2)
{
  minTempHela1 = tempNow1 + 0.2;
  maxTempHela1 = tempNow1 + 0.5;
  minTempLamp1 = tempNow1 - 0.2;
  maxTempLamp1 = tempNow1 - 0.5;

  minTempHela2 = tempNow2 + 0.2;
  maxTempHela2 = tempNow2 + 0.5;
  minTempLamp2 = tempNow2 - 0.2;
  maxTempLamp2 = tempNow2 - 0.5;
}

void subRoutine1Online()
{
  setTemp1 = getServerTemp(getTemp1, setTemp1);
  setTemp2 = getServerTemp(getTemp2, setTemp2);
  tempsUpdate(setTemp1, setTemp2);
  // readTemp1();
  //  delay(2000);
  //  readTemp2();
  //   if (!isnan(temperature1))
  //   {
  //     updateServerTemp(updateTemp1, SENOR_1_NAME, temperature1);
  //   }
  //   if (!isnan(temperature2))
  //   {
  //     updateServerTemp(updateTemp2, SENOR_2_NAME, temperature2);
  //   }
  //   controlTemp1(minTempHela1, maxTempHela1, minTempLamp1, maxTempLamp1, temperature1);
  //   controlTemp2(minTempHela2, maxTempHela2, minTempLamp2, maxTempLamp2, temperature2);
  //   delay(1000);
}

void subRoutine1Offline()
{
  readTemp1();
  delay(2000);
  readTemp2();
  controlTemp1(minTempHela1, maxTempHela1, minTempLamp1, maxTempLamp1, temperature1);
  controlTemp2(minTempHela2, maxTempHela2, minTempLamp2, maxTempLamp2, temperature2);
  delay(1000);
}

void subRoutine2(void *pvParameters)
{
  while (1)
  {
    while (!timeClient.update())
    {
      timeClient.forceUpdate();
    }
    formattedDate = timeClient.getFormattedTime();

    TempLog1.timestampLog = hourTime;
    TempLog1.temperatureLog = temperature1;
    TempLog2.timestampLog = hourTime;
    TempLog2.temperatureLog = temperature2;
    // Serial.println("Timestamp"); Serial.print(TempLog.timestampLog);
    delay(700);
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(REL1, OUTPUT);
  pinMode(VEN1, OUTPUT);
  pinMode(HEL1, OUTPUT);
  digitalWrite(REL1, HIGH);
  digitalWrite(VEN1, HIGH);
  digitalWrite(HEL1, HIGH);

  pinMode(REL2, OUTPUT);
  pinMode(VEN2, OUTPUT);
  pinMode(HEL2, OUTPUT);
  digitalWrite(REL2, HIGH);
  digitalWrite(VEN2, HIGH);
  digitalWrite(HEL2, HIGH);

  Serial.printf("Firmware v%s\r\n", FIRMWARE_VERSION);

  sensorSetup();
  noWiFi = wifiSetup(WIFI_SSID, WIFI_PASSWORD);
  offlineMode = modeSetup(bdaApiURL);
  if (!offlineMode)
  {
    firebaseSetup();
  }

  // Firebase.begin(&config, &auth);
  // config.fcs.download_buffer_size = 2048;
  // Firebase.reconnectWiFi(true);

  // timeClient.begin();
  // timeClient.setTimeOffset(-10800);

  // if (Firebase.ready() && !taskCompleted)
  // {
  //   taskCompleted = true;
  //   Serial.println("\nChecking for new firmware update available...\n");

  //   if (!Firebase.Storage.downloadOTA(
  //           &fbdo, STORAGE_BUCKET_ID,
  //           FIRMWARE_PATH,
  //           firmwareDownload))
  //   {
  //     Serial.println(fbdo.errorReason());
  //   }
  //   else
  //   {
  //     Serial.printf("Delete file... %s\n", Firebase.Storage.deleteFile(&fbdo, STORAGE_BUCKET_ID, FIRMWARE_PATH) ? "ok" : fbdo.errorReason().c_str());
  //     Serial.println("Restarting...\n\n");
  //     delay(2000);
  //     ESP.restart();
  //   }
  // }

  /*if(!offlineMode){
   xTaskCreatePinnedToCore (
    subRoutine2,
    "Logging",
    20000,
    NULL,
    0,
    NULL,
    1
  );
  }*/
}

void loop()
{
  while (error != true)
  {
    offlineMode = modeSetup(bdaStatusApiURL);
    if (offlineMode == false)
    {
      subRoutine1Online();
      // if there is a new update, download it
      if (checkForUpdate(DEVICE_NAME, FIRMWARE_VERSION))
      {
        downloadAndUpdateFirmware();
      }
    }
    else
    {
      subRoutine1Offline();
    }
  }
}