#ifndef _BDA_FIRMWARE_H
#define _BDA_FIRMWARE_H

#include <bda/env.h>
#include <HTTPClient.h>
#include <Firebase_ESP_Client.h>

// Firebase instance
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variables for Receiving Version and Update URL
String Update_Version = "";
String Firebase_Firmware_Update_URL = "";

void firebaseSetup()
{
    // Configure Firebase
    config.api_key = FIREBASE_API_KEY;
    config.database_url = FIREBASE_RTDB_URL;
    auth.user.email = FIREBASE_USER_EMAIL;
    auth.user.password = FIREBASE_USER_PASSWORD;

    // Initialize Firebase
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

bool checkForUpdate(const char *deviceId, const char *currentFirmwareVersion)
{
    if (Firebase.ready())
    {
        // Read Available update Version Number for the device
        String devicePath = String("/devices/");
        devicePath.concat(deviceId);
        devicePath.concat("/update/");

        String deviceVersionPath = devicePath;
        deviceVersionPath.concat("version");
        if (Firebase.RTDB.getString(&fbdo, deviceVersionPath))
        {
            Update_Version = fbdo.stringData();
            Serial.printf(" ********************Current version: %s********************\r\n", currentFirmwareVersion);
            Serial.printf(" ********************Latest version: %s********************\r\n", Update_Version.c_str());

            char update_version = Update_Version.compareTo(currentFirmwareVersion);

            // if version doesn't match update with cloud version
            if (update_version != 0)
            {
                Serial.printf("********************New version available: %s ********************\r\n", Update_Version.c_str());
                // Read update URL
                String deviceURLPath = devicePath;
                deviceURLPath.concat("url");
                if (Firebase.RTDB.getString(&fbdo, deviceURLPath))
                {
                    Firebase_Firmware_Update_URL = fbdo.stringData();
                    Serial.println(Firebase_Firmware_Update_URL);
                    return true;
                }
                else
                {
                    Serial.printf("********************Firebase.RTDB error %s ********************\r\n", fbdo.errorReason().c_str());
                }
            }
            else
            {
                Serial.println("********************Application is Up To Date********************");
            }
        }
        else
        {
            Serial.printf("********************Firebase.RTDB error %s ********************\r\n", fbdo.errorReason().c_str());
        }
    }

    return false;
}

void downloadAndUpdateFirmware()
{
    // Get the download URL from Firebase
    Serial.print("Firmware URL: ");
    Serial.println(Firebase_Firmware_Update_URL);

    HTTPClient http;
    http.begin(Firebase_Firmware_Update_URL);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        WiFiClient &client = http.getStream();
        int firmwareSize = http.getSize();
        Serial.print("Firmware Size: ");
        Serial.println(firmwareSize);

        if (Update.begin(firmwareSize))
        {
            size_t written = Update.writeStream(client); //////////// Takes Time

            if (Update.size() == written)
            {
                Serial.println("********************Update successfully completed. Rebooting...********************");

                if (Update.end())
                {
                    Serial.println("********************Rebooting...********************");
                    ESP.restart();
                }
                else
                {
                    Serial.print("********************Update failed: ********************");
                    Serial.println(Update.errorString());
                }
            }
            else
            {
                Serial.println("********************Not enough space for OTA.********************");
            }
        }
        else
        {
            Serial.println("********************Failed to begin OTA update.********************");
        }
    }
    else
    {
        Serial.print("********************Failed to download firmware. HTTP code: ********************");
        Serial.println(httpCode);
    }

    http.end();
}

#endif // _BDA_NETWORK_H