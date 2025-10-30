#ifndef _BOSQUESFRAMEWORK_H
#define _BOSQUESFRAMEWORK_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <floatToString.h>

HTTPClient http;

// Regresa bool invertido basado en conexion a red WiFi
bool wifiSetup(const char *ssid, const char *passWifi)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passWifi);
  Serial.printf("Conectando a WiFi: %s ...\n", ssid);

  for (int i = 0; i < 10; i++)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.print('.');
      vTaskDelay(1000);
    }
    else
    {
      Serial.println("Conectado al Wifi \n");
      return false;
      break;
    }
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Error al conectarse al WiFi \n");
    WiFi.disconnect(true, false);
    return true;
  }

  Serial.println(WiFi.localIP());
}

// Checkeo de conexion a internet
bool modeSetup(String url)
{
  const int kNetworkTimeout = 2000;
  const int kNetworkDelay = 1000;

  http.begin(url.c_str());
  unsigned long timeoutStart = millis();

  Serial.printf("\n\nVerificando conexion a Internet:... %s\r\n", url.c_str());

  while (http.connected() &&
         (millis() - timeoutStart) < kNetworkTimeout)
  {
    int response = http.GET();
    Serial.print(response);
    if (response == 200)
    {
      Serial.println(" - Modo de trabajo: ONLINE \n");
      timeoutStart = millis();
      http.end();
      delay(kNetworkDelay);
      return false;
    }
    else
    {
      Serial.println("Modo de trabajo: OFFLINE \n");
      delay(5000);
      http.end();
      return true;
    }
  }
}

// Devuelve temp seteada en servidor
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
    Serial.printf("%i \n", responseCode);
  }
  Serial.printf("CONTROL - Temperatura leida del Servidor: %f \r\n", serverTemp);
  http.end();
  return serverTemp;
}

// Envia temp actual a server
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
    Serial.print(" " + responseCode);
    Serial.println("");
  }
  http.end();
}

#endif