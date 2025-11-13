#ifndef _BDA_NETWORK_H
#define _BDA_NETWORK_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <floatToString.h>

HTTPClient http;

// Regresa bool invertido basado en conexion a red WiFi
bool wifiSetup(const char *ssid, const char *passWifi)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, passWifi);
    Serial.printf("Conectando a WiFi: %s ...\r\n", ssid);

    for (int i = 0; i < 5; i++)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.print('.');
            delay(1000);
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

    // Serial.println(WiFi.localIP());
}

// Check internet connection
bool modeSetup(String url)
{
    const int kNetworkDelay = 1000;

    Serial.printf("\n\nVerificando conexion a Internet:... %s\r\n", url.c_str());

    http.setTimeout(2000);
    http.begin(url);
    int response = http.GET();

    if (response == 200)
    {
        Serial.println("Modo de trabajo: ONLINE");
        http.end();
        delay(kNetworkDelay);
        return false;
    }
    else
    {
        if (response > 0)
        {
            Serial.printf("Modo de trabajo: OFFLINE: %i \r\n", response);
        }
        else
        {
            Serial.println("Modo de trabajo: OFFLINE: sin conexión");
        }
        delay(500);
        http.end();
        return true;
    }
}

// Get temperature from the server
float getServerTemp(String url, float setTemp)
{
    float serverTemp = setTemp;

    http.begin(url);
    int responseCode = http.GET();
    String responseTemp = http.getString();

    if (responseCode == 200)
    {
        serverTemp = responseTemp.toFloat();
    }
    else
    {
        Serial.printf("getServerTemp-Error: %i \r\n", responseCode);
    }
    Serial.printf("CONTROL - Temperatura leida del Servidor: %f \r\n", serverTemp);
    http.end();
    return serverTemp;
}

// Send current temperatura to the server
void updateServerTempApp(String url, float newtemp)
{
    char buffer[6];
    String sentTemp = floatToString(newtemp, buffer, 6, 2);

    http.begin(url);
    http.addHeader("temp", sentTemp);
    int responseCode = http.POST("");

    if (responseCode == 200)
    {
        Serial.println("Temperatura enviada correctamente a servidor");
    }
    else
    {
        Serial.printf("updateServerTemp-Error: %i \r\n", responseCode);
    }
    http.end();
}

/**
 * @brief Sends a temperature reading to an InfluxDB instance.
 *
 * @param influxUrl The base URL of your InfluxDB instance (e.g., "http://192.168.1.100:8086").
 * @param influxOrg The organization name in InfluxDB.
 * @param influxBucket The bucket name to write data to.
 * @param influxToken The authentication token for InfluxDB.
 * @param deviceName A name for the device sending the data (e.g., "simple-seed-incubator").
 * @param sensorName A name for the sensor sending the data (e.g., "sensor-1").
 * @param temperature The temperature value to send.
 */
void sendToInfluxDB(const char *influxUrl, const char *influxOrg, const char *influxBucket, const char *influxToken, const char *deviceName, const char *sensorName, float temperature)
{
    HTTPClient http;
    http.setTimeout(2000);

    // Construct the InfluxDB API URL using .concat() to avoid ambiguity
    String url(influxUrl);
    url.concat("/api/v2/write?org=");
    url.concat(influxOrg);
    url.concat("&bucket=");
    url.concat(influxBucket);
    url.concat("&precision=s");

    // Start the HTTP client
    http.begin(url.c_str());

    // Construct the Authorization header using .concat()
    String authToken("Token ");
    authToken.concat(influxToken);
    http.addHeader("Authorization", authToken);
    http.addHeader("Content-Type", "text/plain; charset=utf-8");

    // Prepare the data in InfluxDB's Line Protocol format using .concat()
    String postData("temperature,device=");
    postData.concat(deviceName);
    postData.concat(",location=lab-bda-umepay");
    postData.concat(",sensor=");
    postData.concat(sensorName);
    postData.concat(" value=");
    postData.concat(String(temperature)); // Convert float to String

    Serial.printf("Sending data to InfluxDB: %s - %s\r\n", url.c_str(), postData.c_str());

    // Make the POST request
    int httpResponseCode = http.POST(postData);

    // Check the response
    if (httpResponseCode > 0)
    {
        Serial.printf("InfluxDB response code: %d\n", httpResponseCode);
        // Serial.printf("InfluxDB response: %s\r\n", http.getString().c_str());
    }
    else
    {
        Serial.printf("Error sending to InfluxDB. HTTP error: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    // End the connection
    http.end();
}

// Send current temperatura to the server
void updateServerTemp(const char *deviceName, const char *sensorName, float temperature)
{
    sendToInfluxDB(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, deviceName, sensorName, temperature);
}

// Send current temperatura to the server
void updateServerTemp(String url, const char *sensorName, float newtemp)
{
    updateServerTempApp(url, newtemp);
    updateServerTemp(DEVICE_NAME, sensorName, newtemp);
}

#endif // _BDA_NETWORK_H
