
#include <WiFi.h>
#include <HTTPClient.h>
#include <floatToString.h>

// Regresa bool invertido basado en conexion a red WiFi
bool wifiSetup(const char *ssid, const char *passWifi)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, passWifi);
    Serial.printf("Conectando a WiFi: %s ...\r\n", ssid);

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