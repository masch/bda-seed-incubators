#include <env.h>
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

bool checkForUpdate(const char *currentFirmwareVersion)
{
    if (Firebase.ready())
    {
        // Read Available update Version Number
        if (Firebase.RTDB.getString(&fbdo, "/update/version"))
        {
            Update_Version = fbdo.stringData();
            Serial.printf(" ********************Latest version: %s********************\r\n", Update_Version.c_str());

            char update_version = Update_Version.compareTo(currentFirmwareVersion);

            // if Version Higher than current version
            if (update_version > 0)
            {
                Serial.printf("********************New version available: %s ********************\r\n", Update_Version.c_str());
                // Read update URL
                if (Firebase.RTDB.getString(&fbdo, "/update/url"))
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
            else if (0 == update_version)
            {
                Serial.println("********************Application is Up To Date********************");
            }
            else
            {
                Serial.println("********************Firebase version is old!********************");
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
