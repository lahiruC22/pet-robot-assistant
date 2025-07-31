#include <Arduino.h>
#include "config.h"
#include "communication/wifi_manager.h"
#include "communication/websocket_client.h"
<<<<<<< HEAD
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
=======

WiFiManager wifiManager;
ElevenLabsClient elevenLabs;

// Example callback functions
void onConversationInit(const char* conversation_id) {
    Serial.println("Conversation initialized with ID: " + String(conversation_id));
}

void onAgentResponse(const char* response) {
    Serial.println("Agent says: " + String(response));
    // You can handle the text response here (e.g., display on screen, process commands)
}

void onTranscript(const char* transcript) {
    Serial.println("User said: " + String(transcript));
}

void onAudioData(const uint8_t* data, size_t length, uint32_t event_id) {
    Serial.printf("Received audio chunk: %u bytes (event_id: %u)\n", length, event_id);
    // Since we're overriding audio, you can process this audio data here
    // For example, play it through your speaker system
}

void onToolCall(const char* tool_name, const char* tool_call_id, const JsonDocument& parameters) {
    Serial.println("Tool call requested: " + String(tool_name) + " (ID: " + String(tool_call_id) + ")");
    
    // Example tool handling - you should implement actual tool logic here
    String result = "Tool executed successfully";
    elevenLabs.sendToolResult(tool_call_id, result.c_str(), false);
}

void onError(const char* error_message) {
    Serial.println("ElevenLabs Error: " + String(error_message));
    
    // If we get a connection error, try to restart the connection
    if (String(error_message).indexOf("connection") != -1 || 
        String(error_message).indexOf("disconnect") != -1) {
        Serial.println("Connection error detected, will attempt reconnection...");
    }
}

void onVadScore(float vad_score) {
    // Voice Activity Detection score - useful for detecting when user is speaking
    if (vad_score > 0.5) {
        Serial.printf("Voice detected (VAD: %.2f)\n", vad_score);
    }
}

void onPing(uint32_t event_id, uint32_t ping_ms) {
    Serial.printf("Ping received: %u ms (event_id: %u)\n", ping_ms, event_id);
>>>>>>> upstream/feature/WIZ-53-websocket-connection
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Pet Robot Assistant...");

    // Connect to Wi-Fi using credentials from config.h
    if (!wifiManager.connect(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("Failed to connect to Wi-Fi.");
        while(true) {
            delay(1000);
        }
    }

<<<<<<< HEAD
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
=======
    Serial.println("Wi-Fi connected successfully!");

    // Configure ElevenLabs client
    elevenLabs.setOverrideAudio(true); // Override default audio to handle it ourselves

    // Register callbacks
    elevenLabs.onConversationInit(onConversationInit);
    elevenLabs.onAgentResponse(onAgentResponse);
    elevenLabs.onTranscript(onTranscript);
    elevenLabs.onAudioData(onAudioData);
    elevenLabs.onToolCall(onToolCall);
    elevenLabs.onError(onError);
    elevenLabs.onVadScore(onVadScore);
    elevenLabs.onPing(onPing);

    // Start ElevenLabs WebSocket connection
    elevenLabs.begin(ELEVEN_LABS_AGENT_ID);

    Serial.println("Setup complete!");
}

void loop() {
    // Periodically check Wi-Fi connection status
    if (!wifiManager.isConnected()) {
        Serial.println("Wi-Fi disconnected, attempting to reconnect...");
        if (!wifiManager.connect(WIFI_SSID, WIFI_PASSWORD)) {
            Serial.println("Wi-Fi reconnection failed, retrying in 5 seconds...");
            delay(5000);
            return;
        }
        Serial.println("Wi-Fi reconnected successfully!");
    }

    // Process WebSocket events
    elevenLabs.loop();

    // Handle serial input for interactive messaging
    if (Serial.available()) {
        String userInput = Serial.readStringUntil('\n');
        userInput.trim();
        
        if (userInput.length() > 0) {
            if (elevenLabs.isConnected()) {
                Serial.println("You: " + userInput);
                elevenLabs.sendText(userInput.c_str());
            } else {
                Serial.println("Not connected to ElevenLabs. Please wait for connection...");
            }
        }
    }

    // Send test message and provide status updates
    static unsigned long lastTest = 0;
    static unsigned long lastStatusUpdate = 0;
    static bool testSent = false;
    
    // Status update every 30 seconds
    if (millis() - lastStatusUpdate > 30000) {
        Serial.printf("Status - WiFi: %s, WebSocket: %s\n", 
                     wifiManager.isConnected() ? "Connected" : "Disconnected",
                     elevenLabs.isConnected() ? "Connected" : "Disconnected");
        lastStatusUpdate = millis();
    }
    
    if (!testSent && millis() - lastTest > 10000 && elevenLabs.isConnected()) {
        elevenLabs.sendText("Hello, can you hear me?");
        testSent = true;
        Serial.println("Test message sent! You can now type messages in the serial monitor.");
    }

    delay(10); // Small delay to prevent watchdog issues
>>>>>>> upstream/feature/WIZ-53-websocket-connection
}