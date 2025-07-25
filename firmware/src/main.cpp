#include <Arduino.h>
#include "config.h"
#include "communication/wifi_manager.h"
#include "communication/websocket_client.h"
#include <time.h>

WiFiManager wifiManager;
ElevenLabsClient elevenLabsClient;

// Setup NTP for TLS certificate validation (required for SSL)
void setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync...");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo) || timeinfo.tm_year < (2023 - 1900)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" done.");
  Serial.print(asctime(&timeinfo));
}

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

  // Set up NTP for time synchronization
  setClock();
  Serial.println("Clock setup Completed");

  // Initialize ElevenLabs client
  elevenLabsClient.onConnection([](bool connected) {
    if (connected){
      Serial.println("[MAIN] WebSocket Connected! Sending initial message...");
      elevenLabsClient.sendText("Hello, Who are you?");
    } else {
      Serial.println("[MAIN] WebSocket Disconnected!");
    }
  }); // Close lambda and onConnection call

  elevenLabsClient.onMessage([](const char* type, const char* text){
    if (strcmp(type, "reply") == 0) {
      Serial.println("\nAI Reply:");
      Serial.println(text);
      Serial.println("End of AI Reply\n");
    } else {
      Serial.printf("[MAIN] Received message of type '%s': %s\n", type, text);
    }
  });

  // Start the WebSocket client
  elevenLabsClient.begin(ELEVEN_LABS_AGENT_ID, ELEVEN_LABS_API_KEY);

  Serial.println("[SETUP] Ready.");

} // Close setup()

void loop() {
  // Periodically check Wi-Fi connection status
  if (!wifiManager.isConnected()){
    Serial.println("Wi-Fi disconnected, attempting to reconnect...");
    wifiManager.connect(WIFI_SSID, WIFI_PASSWORD);
  }
  delay(1000); // Delay to avoid flooding the serial output

  elevenLabsClient.loop();

  // Example of sending a message every 30 seconds
  static unsigned long lastMessageTime = 0;
  if (elevenLabsClient.isConnected() && millis() - lastMessageTime > 30000) {
    elevenLabsClient.sendText("Are you still there?");
    lastMessageTime = millis();
  }
}