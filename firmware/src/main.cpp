#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "communication/wifi_manager.h"
#include "communication/websocket_client.h"
#include "audio/microphone.h"
#include "speaker/speaker.h"

// Global instances
WiFiManager wifiManager;
ElevenLabsClient elevenLabsClient;
Microphone microphone;
Speaker speaker;

// Conversation state management
enum ConversationState {
    IDLE,
    CONNECTING,
    WAITING_FOR_TRIGGER,
    COUNTDOWN,
    RECORDING,
    PROCESSING_AUDIO,
    WAITING_FOR_RESPONSE,
    PLAYING_RESPONSE,
    ERROR_STATE
};

ConversationState currentState = IDLE;
unsigned long stateTimer = 0;
int countdownSeconds = 0;
bool autoMode = false;  // Manual trigger vs auto conversation mode

// Function declarations
void initializeHardware();
void initializeElevenLabs();
void handleSerialInput();
void handleConversationFlow();
void changeState(ConversationState newState);
void startRecordingSequence();
void handleCountdown();
void processRecordedAudio();
void setupElevenLabsCallbacks();

// ElevenLabs event handlers
void onConversationInit(const char* conversation_id);
void onAgentResponse(const char* response);
void onAudioData(const String& base64Audio, uint32_t event_id);
void onError(const char* error_message);
void onTranscript(const char* transcript);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n" + String("=").substring(0, 50));
    Serial.println(" PET ROBOT ASSISTANT - CONVERSATIONAL AI");
    Serial.println(" Real-time Voice Conversation System");
    Serial.println(" ElevenLabs Integration");
    Serial.println(String("=").substring(0, 50));
    
    // Initialize all hardware components
    initializeHardware();
    
    // Setup ElevenLabs callbacks
    setupElevenLabsCallbacks();
    
    // Initialize ElevenLabs connection
    initializeElevenLabs();
    
    Serial.println("\n" + String("=").substring(0, 50));
    Serial.println("READY FOR CONVERSATION!");
    Serial.println("Commands:");
    Serial.println("  'r' + Enter: Start single recording");
    Serial.println("  'a' + Enter: Toggle auto conversation mode");
    Serial.println("  's' + Enter: Stop current operation");
    Serial.println("  'v' + Enter: Adjust speaker volume");
    Serial.println(String("=").substring(0, 50) + "\n");
    
    changeState(WAITING_FOR_TRIGGER);
}

void loop() {
    // Handle WebSocket communication
    if (elevenLabsClient.isConnected()) {
        elevenLabsClient.loop();
    }
    
    // Handle audio systems
    microphone.loop();
    speaker.loop();
    
    // Handle serial commands
    handleSerialInput();
    
    // Main conversation flow state machine
    handleConversationFlow();
    
    yield();  // Allow background tasks to run without introducing latency
    delay(1); // Add a small delay to prevent excessive CPU usage
}

void initializeHardware() {
    Serial.println("Initializing WiFi...");
    if (!wifiManager.connect(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("Failed to connect to WiFi. Restarting...");
        ESP.restart();
    }
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    
    Serial.println("Initializing microphone...");
    if (!microphone.begin(MIC_SAMPLE_RATE)) {
        Serial.println("Failed to initialize microphone!");
        changeState(ERROR_STATE);
        return;
    }
    Serial.println("Microphone initialized");
    
    Serial.println("Initializing speaker...");
    if (!speaker.begin(SPEAKER_SAMPLE_RATE)) {
        Serial.println("Failed to initialize speaker!");
        changeState(ERROR_STATE);
        return;
    }
    Serial.println("Speaker initialized");
    speaker.setVolume(0.7f);  // Set default volume to 70%
}

void setupElevenLabsCallbacks() {
    elevenLabsClient.onConversationInit(onConversationInit);
    elevenLabsClient.onAgentResponse(onAgentResponse);
    elevenLabsClient.onAudioData(onAudioData);
    elevenLabsClient.onError(onError);
    elevenLabsClient.onTranscript(onTranscript);
}

void initializeElevenLabs() {
    Serial.println("Connecting to ElevenLabs...");
    changeState(CONNECTING);
    
    // Initialize WebSocket connection
    elevenLabsClient.begin(ELEVEN_LABS_AGENT_ID);
    
    // Wait for connection with timeout
    unsigned long connectionStart = millis();
    bool connected = false;
    
    while (millis() - connectionStart < 15000) {  // 15 second timeout
        elevenLabsClient.loop();
        
        if (elevenLabsClient.isConnected()) {
            connected = true;
            break;
        }
        delay(100);
    }
    
    if (connected) {
        Serial.println("ElevenLabs connected successfully");
    } else {
        Serial.println("Failed to connect to ElevenLabs");
        changeState(ERROR_STATE);
    }
}

void handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toLowerCase();
        
        if (input == "r") {
            if (currentState == WAITING_FOR_TRIGGER) {
                Serial.println("Starting recording sequence...");
                startRecordingSequence();
            } else {
                Serial.println("Can't start recording in current state");
            }
        }
        else if (input == "a") {
            autoMode = !autoMode;
            Serial.println("Auto conversation mode: " + String(autoMode ? "ON" : "OFF"));
            
            if (autoMode && currentState == WAITING_FOR_TRIGGER) {
                Serial.println("Starting auto conversation...");
                startRecordingSequence();
            }
        }
        else if (input == "s") {
            Serial.println("Stopping current operation...");
            speaker.stop();
            microphone.clearBuffer();
            changeState(WAITING_FOR_TRIGGER);
        }
        else if (input == "v") {
            float currentVolume = speaker.getVolume();
            float newVolume = (currentVolume >= 1.0f) ? 0.3f : currentVolume + 0.2f;
            speaker.setVolume(newVolume);
            Serial.printf("Speaker volume: %.1f%%\n", newVolume * 100);
        }
        else if (input.length() > 0) {
            Serial.println("Unknown command: " + input);
        }
    }
}

void handleConversationFlow() {
    switch (currentState) {
        case WAITING_FOR_TRIGGER:
            // Idle state - waiting for user input
            break;
            
        case COUNTDOWN:
            handleCountdown();
            break;
            
        case RECORDING:
            if (microphone.isRecordingComplete()) {
                Serial.println("Recording complete!");
                changeState(PROCESSING_AUDIO);
            }
            break;
            
        case PROCESSING_AUDIO:
            processRecordedAudio();
            break;
            
        case WAITING_FOR_RESPONSE:
            // Waiting for agent response and audio
            break;
            
        case PLAYING_RESPONSE:
            if (!speaker.isPlaying()) {
                Serial.println("Response playback complete!");
                
                if (autoMode) {
                    Serial.println("Auto mode: Starting next recording cycle...");
                    delay(1000);  // Brief pause before next recording
                    startRecordingSequence();
                } else {
                    changeState(WAITING_FOR_TRIGGER);
                }
            }
            break;
            
        case ERROR_STATE:
            // Error state - system halted
            Serial.println("System in error state. Reset required.");
            delay(5000);
            break;
            
        default:
            break;
    }
}

void changeState(ConversationState newState) {
    if (currentState != newState) {
        currentState = newState;
        stateTimer = millis();
        
        // Optional: Print state changes for debugging
        // Serial.printf("[STATE] Changed to: %d\n", (int)newState);
    }
}

void startRecordingSequence() {
    Serial.println("Starting 3-second countdown...");
    countdownSeconds = 3;
    changeState(COUNTDOWN);
}

void handleCountdown() {
    if (millis() - stateTimer >= 1000) {  // Every second
        Serial.printf("Recording in: %d\n", countdownSeconds);
        countdownSeconds--;
        stateTimer = millis();
        
        if (countdownSeconds <= 0) {
            Serial.println("RECORDING NOW!");
            if (microphone.startRecording(3)) {  // 3-second recording
                changeState(RECORDING);
            } else {
                Serial.println("Failed to start recording!");
                changeState(ERROR_STATE);
            }
        }
    }
}

void processRecordedAudio() {
    String audioBase64 = microphone.getBase64AudioData();
    
    if (audioBase64.length() > 0) {
        Serial.printf("Sending audio (%d chars) to ElevenLabs...\n", audioBase64.length());
        
        // Send audio to ElevenLabs
        elevenLabsClient.sendAudio(audioBase64.c_str());
        
        // Clear microphone buffer
        microphone.clearBuffer();
        
        changeState(WAITING_FOR_RESPONSE);
    } else {
        Serial.println("No audio data recorded!");
        changeState(WAITING_FOR_TRIGGER);
    }
}

// ElevenLabs Event Handlers
void onConversationInit(const char* conversation_id) {
    Serial.println("Conversation initialized: " + String(conversation_id));
}

void onAgentResponse(const char* response) {
    Serial.println("\n" + String("=").substring(0, 40));
    Serial.println("AGENT RESPONSE:");
    Serial.println(response);
    Serial.println(String("=").substring(0, 40));
}

void onAudioData(const String& base64Audio, uint32_t event_id) {
    Serial.printf("Received audio response (Event: %u, %d chars)\n", event_id, base64Audio.length());
    
    // Play the audio through speaker
    if (speaker.playBase64Audio(base64Audio)) {
        Serial.println("Playing agent response audio...");
        changeState(PLAYING_RESPONSE);
    } else {
        Serial.println("Failed to start audio playback!");
        if (autoMode) {
            // Continue conversation even if audio fails
            delay(2000);
            startRecordingSequence();
        } else {
            changeState(WAITING_FOR_TRIGGER);
        }
    }
}

void onTranscript(const char* transcript) {
    Serial.println("User transcript: " + String(transcript));
}

void onError(const char* error_message) {
    Serial.println("ElevenLabs Error: " + String(error_message));
    
    // Attempt to recover from errors
    if (currentState == WAITING_FOR_RESPONSE) {
        Serial.println("Attempting to recover...");
        delay(2000);
        changeState(WAITING_FOR_TRIGGER);
    }
}