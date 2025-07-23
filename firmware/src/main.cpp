#include <Arduino.h>
#include "config.h"
#include "communication/wifi_manager.h"

WiFiManager wifiManager;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi using credentials from config.h
  // The Wokwi guest network doesn't require a password.
  if (!wifiManager.connect(WIFI_SSID, WIFI_PASSWORD)){
    Serial.println("Failed to connect to Wi-Fi.");

    // Handle connection failure
    while(true) {
      delay(1000);
    }
  }

  // -- All other setup code follows here --
  Serial.println("Wi-Fi connected successfully!");
}

void loop() {
  // Periodically check Wi-Fi connection status
  if (!wifiManager.isConnected()){
    Serial.println("Wi-Fi disconnected, attempting to reconnect...");
    wifiManager.connect(WIFI_SSID, WIFI_PASSWORD);
  }
  delay(1000); // Delay to avoid flooding the serial output
}