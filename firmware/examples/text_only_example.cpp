#include <Arduino.h>
#include "config.h"
#include "communication/wifi_manager.h"
#include "communication/websocket_client.h"

WiFiManager wifiManager;
ElevenLabsClient elevenLabs;

// Simple text-only example
void onAgentResponse(const char* response) {
    Serial.println("Agent: " + String(response));
}

void onError(const char* error_message) {
    Serial.println("Error: " + String(error_message));
}

void onConversationInit(const char* conversation_id) {
    Serial.println("Conversation started with ID: " + String(conversation_id));
    
    // Send initial message once conversation is ready
    elevenLabs.sendText("Hello! Can you introduce yourself?");
}

void setup() {
    Serial.begin(115200);
    Serial.println("ElevenLabs Text-Only Example");

    // Connect to Wi-Fi
    if (!wifiManager.connect(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("Failed to connect to Wi-Fi.");
        while(true) delay(1000);
    }

    Serial.println("Wi-Fi connected!");

    // Configure ElevenLabs client for text-only communication
    elevenLabs.setOverrideAudio(true); // We don't want audio in this example
    elevenLabs.setCustomPrompt("You are a helpful assistant. Keep responses concise and friendly.");
    elevenLabs.setFirstMessage("Hello! I'm ready to help you.");

    // Register only the callbacks we need
    elevenLabs.onConversationInit(onConversationInit);
    elevenLabs.onAgentResponse(onAgentResponse);
    elevenLabs.onError(onError);

    // Start connection
    elevenLabs.begin(ELEVEN_LABS_API_KEY, ELEVEN_LABS_AGENT_ID);
    
    Serial.println("Connecting to ElevenLabs...");
}

void loop() {
    // Check Wi-Fi connection
    if (!wifiManager.isConnected()) {
        Serial.println("Wi-Fi disconnected, reconnecting...");
        wifiManager.connect(WIFI_SSID, WIFI_PASSWORD);
    }

    // Process WebSocket events
    elevenLabs.loop();

    // Example: Send messages at intervals for testing
    static unsigned long lastMessage = 0;
    static int messageCount = 0;
    static const char* testMessages[] = {
        "What can you help me with?",
        "Tell me a joke",
        "What's the weather like?",
        "Goodbye"
    };

    if (elevenLabs.isConnected() && millis() - lastMessage > 15000) { // Every 15 seconds
        if (messageCount < 4) {
            Serial.println("Sending: " + String(testMessages[messageCount]));
            elevenLabs.sendText(testMessages[messageCount]);
            messageCount++;
            lastMessage = millis();
        }
    }

    delay(10);
}
