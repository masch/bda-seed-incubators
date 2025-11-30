#include <Arduino.h>
#include <DHT.h>
#include <Firebase_ESP_Client.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <bda/env.h>
#include <bda/firmware.h>
#include <bda/network.h>
#include <floatToString.h>
#include <math.h>
#include <stdlib.h>

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
const char* SENOR_1_NAME = "S1";
DHT sensor2(S2, DHTTYPE);
const char* SENOR_2_NAME = "S2";

// WiFi Definitions
const String bdaApiURL = BDA_API_URL;
String bdaStatusApiURL = bdaApiURL + "/status";
String getTemp1 = bdaApiURL + "/v1/heladera-doble-1/gettemp";
String updateTemp1 = bdaApiURL + "/v1/heladera-doble-1/updatetemp";
String getTemp2 = bdaApiURL + "/v1/heladera-doble-2/gettemp";
String updateTemp2 = bdaApiURL + "/v1/heladera-doble-2/updatetemp";

unsigned long lastMillis;

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

void sensorSetup() {
  sensor1.begin();
  sensor2.begin();
}

void readTemp1() {
  delay(1000);
  temp1 = sensor1.readTemperature();

  if (isnan(temp1)) {
    Serial.println("****ERROR****: Failed to read temperature from sensor 1");
    temperature1 = NAN;
    return;
  }

  temperature1 = temp1;
  Serial.print("INFO - Temperatura detectada sensor: ");
  Serial.println(temperature1);
}

void readTemp2() {
  delay(1000);
  temp2 = sensor2.readTemperature();

  if (isnan(temp2)) {
    Serial.println("****ERROR****: Failed to read temperature from sensor 2");
    temperature2 = NAN;
    return;
  }
  temperature2 = temp2;
  Serial.print("INFO - Temperatura detectada sensor: ");
  Serial.println(temperature2);
}

void controlTemp1(float minHela, float maxHela, float minLamp, float maxLamp, float tempNow) {
  if (isnan(tempNow)) {
    Serial.println("Temperatura leida invalida Heladera 1");
    digitalWrite(REL1, HIGH);
    digitalWrite(VEN1, HIGH);
    digitalWrite(HEL1, HIGH);
    heladeraOn1 = false;
    lampOn1 = false;
  } else {
    Serial.print("CONTROL - Temperatura de trabajo Heladera Doble 1: ");
    Serial.println(setTemp1);
    if (tempNow >= minHela) {
      if (tempNow >= maxHela) {
        digitalWrite(HEL1, LOW);
        digitalWrite(VEN1, LOW);
        digitalWrite(REL1, HIGH);
        heladeraOn1 = true;
        Serial.println("Entra en Modo encender heladera - Heladera 1");
      } else {
        if (tempNow <= minHela && heladeraOn1) {
          digitalWrite(HEL1, HIGH);
          digitalWrite(VEN1, HIGH);
          heladeraOn1 = false;
          Serial.println("Entra en Modo apagar heladera - Heladera 1");
        }
      }
    }

    else if (tempNow <= minLamp) {
      if (tempNow <= maxLamp) {
        digitalWrite(REL1, LOW);
        digitalWrite(VEN1, LOW);
        digitalWrite(HEL1, HIGH);
        lampOn1 = true;
        Serial.println("Entra en Modo encender lampara - Heladera 1");
      } else {
        if (tempNow >= minLamp && lampOn1) {
          digitalWrite(REL1, HIGH);
          digitalWrite(VEN1, HIGH);
          lampOn1 = false;
          Serial.println("Entra en Modo apagar lampara Heladera 1");
        }
      }
    }

    else if (!lampOn1 || !heladeraOn1) {
      digitalWrite(REL1, HIGH);
      digitalWrite(VEN1, HIGH);
      digitalWrite(HEL1, HIGH);
      Serial.println("Entra en Modo apagar todo - Heladera 1");
    }
  }
}

void controlTemp2(float minHela, float maxHela, float minLamp, float maxLamp, float tempNow) {
  if (isnan(tempNow)) {
    Serial.println("Temperatura leida invalida Heladera 2");
    digitalWrite(REL2, HIGH);
    digitalWrite(VEN2, HIGH);
    digitalWrite(HEL2, HIGH);
    heladeraOn2 = false;
    lampOn2 = false;
  } else {
    Serial.print("CONTROL - Temperatura de trabajo Heladera Doble 2: ");
    Serial.println(setTemp2);
    if (tempNow >= minHela) {
      if (tempNow >= maxHela) {
        digitalWrite(HEL2, LOW);
        digitalWrite(VEN2, LOW);
        digitalWrite(REL2, HIGH);
        heladeraOn2 = true;
        Serial.println("Entra en Modo encender heladera - Heladera 2");
      } else {
        if (tempNow <= minHela && heladeraOn2) {
          digitalWrite(HEL2, HIGH);
          digitalWrite(VEN2, HIGH);
          heladeraOn2 = false;
          Serial.println("Entra en Modo apagar heladera - Heladera 2");
        }
      }
    }

    else if (tempNow <= minLamp) {
      if (tempNow <= maxLamp) {
        digitalWrite(REL2, LOW);
        digitalWrite(VEN2, LOW);
        digitalWrite(HEL2, HIGH);
        lampOn2 = true;
        Serial.println("Entra en Modo encender lampara - Heladera 2");
      } else {
        if (tempNow >= minLamp && lampOn2) {
          digitalWrite(REL2, HIGH);
          digitalWrite(VEN2, HIGH);
          lampOn2 = false;
          Serial.println("Entra en Modo apagar lampara - Heladera 2");
        }
      }
    }

    else if (!lampOn2 || !heladeraOn2) {
      digitalWrite(REL2, HIGH);
      digitalWrite(VEN2, HIGH);
      digitalWrite(HEL2, HIGH);
      Serial.println("Entra en Modo apagar todo - Heladera 2");
    }
  }
}

void tempsUpdate(float tempNow1, float tempNow2) {
  minTempHela1 = tempNow1 + 0.2;
  maxTempHela1 = tempNow1 + 0.5;
  minTempLamp1 = tempNow1 - 0.2;
  maxTempLamp1 = tempNow1 - 0.5;

  minTempHela2 = tempNow2 + 0.2;
  maxTempHela2 = tempNow2 + 0.5;
  minTempLamp2 = tempNow2 - 0.2;
  maxTempLamp2 = tempNow2 - 0.5;
}

void subRoutine1Online() {
  setTemp1 = getServerTemp(getTemp1, setTemp1);
  setTemp2 = getServerTemp(getTemp2, setTemp2);
  tempsUpdate(setTemp1, setTemp2);
  readTemp1();
  delay(500);
  readTemp2();
  if (!isnan(temperature1)) {
    updateServerTemp(updateTemp1, SENOR_1_NAME, temperature1);
  }
  if (!isnan(temperature2)) {
    updateServerTemp(updateTemp2, SENOR_2_NAME, temperature2);
  }
  controlTemp1(minTempHela1, maxTempHela1, minTempLamp1, maxTempLamp1, temperature1);
  controlTemp2(minTempHela2, maxTempHela2, minTempLamp2, maxTempLamp2, temperature2);
  delay(1000);
}

void subRoutine1Offline() {
  readTemp1();
  delay(500);
  readTemp1();
  controlTemp1(minTempHela1, maxTempHela1, minTempLamp1, maxTempLamp1, temperature1);
  controlTemp2(minTempHela2, maxTempHela2, minTempLamp2, maxTempLamp2, temperature2);
  delay(1000);
}

void setup() {
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
  offlineMode = modeSetup(bdaStatusApiURL);
  timeClient.begin();
  timeClient.setTimeOffset(-10800);

  Serial.print("Syncing time");
  while (!timeClient.update()) {
    timeClient.forceUpdate();
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println(timeClient.getFormattedTime());

  if (!offlineMode) {
    firebaseSetup();
  }
}

void loop() {
  offlineMode = modeSetup(bdaStatusApiURL);
  updateServerAlive();
  if (offlineMode == false) {
    subRoutine1Online();
    // if there is a new update, download it
    if (checkForUpdate(DEVICE_NAME, FIRMWARE_VERSION)) {
      downloadAndUpdateFirmware();
    }
  } else {
    subRoutine1Offline();
  }
  delay(2000);
}