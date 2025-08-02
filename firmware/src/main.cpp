#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "communication/wifi_manager.h"
#include "communication/websocket_client.h"
#include "audio/microphone.h"

// Function declarations  
void startRecordingSequence();
void handleCountdown();
void startRecording();
void handleRecordingComplete();
void connectAndSendAudio(String audioBase64);
void resetState();

// Global instances
WiFiManager wifiManager;
ElevenLabsClient elevenLabsClient;
Microphone microphone;

// State management
bool isWaitingForRecord = false;
bool isCountingDown = false;
bool isRecording = false;
bool isWaitingForResponse = false;
unsigned long countdownStart = 0;
unsigned long recordingStart = 0;
int countdownSeconds = 5;
int recordingSeconds = 3;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== Pet Robot Assistant Starting ===");
    
    // Initialize WiFi
    Serial.println("Initializing WiFi...");
    if (!wifiManager.connect(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("Failed to connect to WiFi. Restarting...");
        ESP.restart();
    }
    
    Serial.println("WiFi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Initialize microphone
    Serial.println("Initializing microphone...");
    if (!microphone.begin(MIC_SAMPLE_RATE)) {
        Serial.println("Failed to initialize microphone!");
        return;
    }
    Serial.println("Microphone initialized successfully!");
    
    Serial.println("\n=== Ready for Commands ===");
    Serial.println("Type 'r' and press Enter to start recording sequence");
    Serial.println("=======================================\n");
}

void loop() {
    // Handle WebSocket connection (only when needed)
    if (elevenLabsClient.isConnected()) {
        elevenLabsClient.loop();
    }
    
    // Handle microphone recording process
    microphone.loop();
    
    // Handle serial input for 'r' command
    if (Serial.available() && !isWaitingForRecord && !isCountingDown && !isRecording) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.equals("r") || input.equals("R")) {
            startRecordingSequence();
        }
    }
    
    // Handle countdown
    if (isCountingDown) {
        handleCountdown();
    }
    
    // Handle recording completion
    if (isRecording && microphone.isRecordingComplete()) {
        handleRecordingComplete();
    }
    
    delay(10);
}

void startRecordingSequence() {
    Serial.println("\n=== Starting Recording Sequence ===");
    Serial.println("Preparing for 5-second countdown...");
    
    isWaitingForRecord = true;
    isCountingDown = true;
    countdownStart = millis();
    countdownSeconds = 5;
    
    Serial.printf("Countdown: %d seconds remaining\n", countdownSeconds);
}

void handleCountdown() {
    unsigned long elapsed = millis() - countdownStart;
    
    if (elapsed >= 1000) { // Every second
        countdownSeconds--;
        countdownStart = millis();
        
        if (countdownSeconds > 0) {
            Serial.printf("Countdown: %d seconds remaining\n", countdownSeconds);
        } else {
            // Start recording
            Serial.println("\n Starting 3-second recording NOW!");
            startRecording();
        }
    }
}

void startRecording() {
    isCountingDown = false;
    isRecording = true;
    recordingStart = millis();
    
    // Start 3-second recording
    if (microphone.startRecording(3)) {
        Serial.println("Recording started successfully!");
    } else {
        Serial.println("Failed to start recording!");
        resetState();
    }
}

void handleRecordingComplete() {
    Serial.println("Recording complete!");
    isRecording = false;
    
    // Get base64 encoded audio data from PSRAM
    String audioBase64 = microphone.getBase64AudioData();
    
    if (audioBase64.length() > 0) {
        Serial.printf("Audio recorded: %d characters base64 data\n", audioBase64.length());
        
        // Print base64 data to serial for verification
        Serial.println("\n=== Base64 Audio Data ===");
        Serial.println(audioBase64);
        Serial.println("=== End Base64 Data ===\n");
        
        // Connect to WebSocket and send audio
        connectAndSendAudio(audioBase64);
    } else {
        Serial.println("Error: No audio data recorded!");
        resetState();
    }
    
    // Clear microphone buffer for next recording
    microphone.clearBuffer();
}

void connectAndSendAudio(String audioBase64) {
    Serial.println("\n=== Connecting to ElevenLabs WebSocket ===");
    
    // Set up callbacks for WebSocket events
    elevenLabsClient.onConversationInit([](const char* conversation_id) {
        Serial.printf("Conversation initialized: %s\n", conversation_id);
        isWaitingForResponse = true;
    });
    
    elevenLabsClient.onAgentResponse([](const char* response) {
        Serial.println("\n" + String("=").substring(0, 50));
        Serial.println("AI AGENT RESPONSE:");
        Serial.println(response);
        Serial.println(String("=").substring(0, 50) + "\n");
    });
    
    elevenLabsClient.onAudioData([](const uint8_t* data, size_t length, uint32_t event_id) {
        // This callback is triggered when audio data is received
        Serial.printf("Received audio chunk: event_id=%u, length=%u\n", event_id, length);
    });
    
    elevenLabsClient.onError([](const char* error_message) {
        Serial.printf("WebSocket Error: %s\n", error_message);
    });
    
    // Initialize WebSocket connection
    elevenLabsClient.begin(ELEVEN_LABS_AGENT_ID);
    
    // Wait for connection and initialization
    Serial.println("Waiting for WebSocket connection and initialization...");
    
    unsigned long connectionStart = millis();
    bool connectionEstablished = false;
    bool conversationInitialized = false;
    
    // Wait up to 15 seconds for connection and initialization
    while (millis() - connectionStart < 15000) {
        elevenLabsClient.loop();
        
        if (!connectionEstablished && elevenLabsClient.isConnected()) {
            Serial.println("WebSocket connected!");
            connectionEstablished = true;
        }
        
        if (connectionEstablished && isWaitingForResponse) {
            Serial.println("Conversation initialized!");
            conversationInitialized = true;
            break;
        }
        
        delay(100);
    }
    
    if (!conversationInitialized) {
        Serial.println("Failed to establish connection or initialize conversation!");
        elevenLabsClient.disconnect();
        resetState();
        return;
    }
    
    // Send the audio data
    Serial.println("\n== Sending Audio Data ==");
    Serial.printf("Sending %d characters of base64 audio data...\n", audioBase64.length());
    
    elevenLabsClient.sendAudio(audioBase64.c_str());
    
    // Listen for responses
    Serial.println("\n=== Listening for Responses ===");
    Serial.println("Waiting for agent response and audio...");
    
    // Listen for 30 seconds for responses
    unsigned long listenStart = millis();
    while (millis() - listenStart < 30000) {
        elevenLabsClient.loop();
        delay(50);
    }
    
    Serial.println("\n=== Session Complete ===");
    elevenLabsClient.disconnect();
    resetState();
    Serial.println("Ready for next recording. Type 'r' to start again.\n");
}

void resetState() {
    isWaitingForRecord = false;
    isCountingDown = false;
    isRecording = false;
    isWaitingForResponse = false;
    countdownSeconds = 5;
    recordingSeconds = 3;
}