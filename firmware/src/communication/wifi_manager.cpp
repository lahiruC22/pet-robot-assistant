#include "wifi_manager.h"
#include <Arduino.h>

bool WiFiManager::connect(const char* ssid, const char* password, unsigned long timeout_ms) {
    Serial.print("Connecting to WiFi SSID: ");
    Serial.println(ssid);

    // Disconnect any existing connection
    WiFi.disconnect(true);
    delay(1000);
    
    // Configure WiFi mode
    WiFi.mode(WIFI_STA);
    
    // For WokWi, the channel (6) is sometimes needed.
    // For physical hardware, this is usually not required.
    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        // Check for the timeout
        if (millis() - startTime > timeout_ms) {
            Serial.println("\nConnection timed out!");
            WiFi.disconnect();
            return false;
        }
        delay(250);
        Serial.print(".");
    }

    Serial.println(" Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    return true;
}

bool WiFiManager::isConnected() {
    return (WiFi.status()==WL_CONNECTED);
}