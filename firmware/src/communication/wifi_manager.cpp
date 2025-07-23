#include "wifi_manager.h"
#include <Arduino.h>

bool WiFiManager::connect(const char* ssid, const char* password, unsigned long timeout_ms){
    Serial.print("Connecting to WiFi SSID: ");
    Serial.println(ssid);

    // For WokWi, the channel (6) is sometimes needed.
    // For physical hardware, this is usually not required.
    WiFi.begin(ssid, password, 6);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        // Check for the timeout
        if (millis() - startTime > timeout_ms) {
            Serial.println("\nConnection timed out!");
            WiFi.disconnect();
            return false;
        }
        delay(100);
        Serial.print(".");
    }

    Serial.println(" Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    return true;
}

bool WiFiManager::isConnected() {
    return (WiFi.status()==WL_CONNECTED);
}