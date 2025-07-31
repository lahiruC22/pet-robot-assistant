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
      Serial.println("[MAIN] ✅ WebSocket Connected! Conversation initiated.");
      Serial.println("🎵 Waiting for agent's initial response and audio chunks...");
    } else {
      Serial.println("[MAIN] ❌ WebSocket Disconnected!");
    }
  }); // Close lambda and onConnection call

  elevenLabsClient.onMessage([](const char* type, const char* text){
    if (strcmp(type, "agent_response") == 0) {
      Serial.println("\n" + String("=").substring(0, 50));
      Serial.println("🤖 AI AGENT RESPONSE:");
      Serial.println(text);
      Serial.println(String("=").substring(0, 50) + "\n");
    } else if (strcmp(type, "user_transcript") == 0) {
      Serial.println("\n" + String("-").substring(0, 30));
      Serial.println("👤 USER TRANSCRIPT:");
      Serial.println(text);
      Serial.println(String("-").substring(0, 30) + "\n");
    } else if (strcmp(type, "error") == 0) {
      Serial.println("\n❌ ERROR MESSAGE:");
      Serial.println(text);
      Serial.println();
    } else {
      Serial.printf("[MAIN] 📨 Received message type '%s': %s\n", type, text);
    }
  });

  // Register audio chunk callback to handle base64 audio chunks
  elevenLabsClient.onAudioChunk([](const char* base64Audio, int eventId, bool isComplete) {
    Serial.printf("[MAIN] 🎵 Audio chunk received - Event ID: %d, Length: %d, Complete: %s\n", 
                eventId, strlen(base64Audio), isComplete ? "Yes" : "No");
    
    if (isComplete) {
      Serial.println("\n🎉 COMPLETE AUDIO STREAM RECEIVED!");
      String completeAudio = elevenLabsClient.getCompleteAudioBase64();
      int totalChunks = elevenLabsClient.getAudioChunkCount();
      
      Serial.printf("📊 Total chunks: %d\n", totalChunks);
      Serial.printf("📏 Total audio length: %d characters\n", completeAudio.length());
      Serial.printf("💾 Estimated decoded size: %d bytes\n", (completeAudio.length() * 3) / 4);
      
      Serial.println("\n" + String("█").substring(0, 60));
      Serial.println("🎵 COMPLETE BASE64 AUDIO DATA:");
      Serial.println(completeAudio);
      Serial.println(String("█").substring(0, 60) + "\n");
      
      // Clear buffer for next audio stream
      elevenLabsClient.clearAudioBuffer();
    }
  });

  // Start the WebSocket client
  elevenLabsClient.begin(ELEVEN_LABS_AGENT_ID);

  Serial.println("[SETUP] Ready.");

} // Close setup()

void loop() {
  // Periodically check Wi-Fi connection status
  if (!wifiManager.isConnected()){
    Serial.println("[MAIN] 📶 Wi-Fi disconnected, attempting to reconnect...");
    wifiManager.connect(WIFI_SSID, WIFI_PASSWORD);
  }

  elevenLabsClient.loop();

  // Debug connection status every 10 seconds
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 10000) {
    Serial.println("\n[DEBUG] Connection Status:");
    elevenLabsClient.getConnectionInfo();
    lastDebugTime = millis();
  }

  // Send conversation messages every 30 seconds if connected
  static unsigned long lastMessageTime = 0;
  static int messageCount = 0;
  
  if (elevenLabsClient.isConnected() && millis() - lastMessageTime > 30000) {
    const char* messages[] = {
      "What's your favorite activity?",
      "Can you tell me about yourself?",
      "What can you help me with?",
      "How are you feeling today?",
      "Tell me something interesting.",
      "What do you think about technology?"
    };
    
    int messageIndex = messageCount % (sizeof(messages) / sizeof(messages[0]));
    
    Serial.printf("\n[MAIN] 📤 Sending message #%d: %s\n", messageCount + 1, messages[messageIndex]);
    elevenLabsClient.sendText(messages[messageIndex]);
    
    lastMessageTime = millis();
    messageCount++;
  }

  // Send user activity every 15 seconds to keep connection alive
  static unsigned long lastActivityTime = 0;
  if (elevenLabsClient.isConnected() && millis() - lastActivityTime > 15000) {
    Serial.println("[MAIN] 💭 Sending user activity to keep connection alive");
    elevenLabsClient.sendUserActivity();
    lastActivityTime = millis();
  }
  
  delay(100); // Small delay to prevent flooding
}