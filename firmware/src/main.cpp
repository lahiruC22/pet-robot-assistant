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
bool realtimeMode = false;  // Real-time streaming vs batch recording

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
void onAudioData(const uint8_t* pcm_data, size_t size, uint32_t event_id);  // Changed to raw PCM
void onError(const char* error_message);
void onTranscript(const char* transcript);
void onInterruption(uint32_t event_id);  // New interrupt handler
void handleAudioPlaybackError();

// Real-time streaming callback (like Python SDK input_callback)
void onRealtimeAudioChunk(const int16_t* audioData, size_t samples);

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
    Serial.println("  't' + Enter: Toggle streaming audio mode");
    Serial.println("  'realtime' + Enter: Toggle real-time streaming mode");
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
    
    // Handle real-time streaming if enabled
    if (realtimeMode && microphone.isRealtimeStreaming()) {
        microphone.realtimeLoop();
    }
    
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
    elevenLabsClient.onInterruption(onInterruption);  // Add interrupt callback
    
    // Enable streaming audio for better responsiveness
    elevenLabsClient.enableStreamingAudio(true);
}

void initializeElevenLabs() {
    Serial.println("Connecting to ElevenLabs...");
    changeState(CONNECTING);
    
    // Initialize WebSocket connection for public agent
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
        
        // If in real-time mode, only allow stopping or volume changes
        if (realtimeMode) {
            if (input == "realtime" || input == "s") {
                Serial.println("[REALTIME] Stopping real-time mode...");
                realtimeMode = false;
                microphone.stopRealtimeStreaming();
                elevenLabsClient.stopRealtimeStreaming();
                changeState(WAITING_FOR_TRIGGER);
                Serial.println("[REALTIME] ✓ Stopped - back to manual mode");
            } else if (input == "v") {
                float currentVolume = speaker.getVolume();
                float newVolume = (currentVolume >= 1.0f) ? 0.3f : currentVolume + 0.2f;
                speaker.setVolume(newVolume);
                Serial.printf("Speaker volume: %.1f%%\n", newVolume * 100);
            } else if (input.length() > 0) {
                Serial.println("[REALTIME] In real-time mode. Use 'realtime' or 's' to stop.");
            }
            return; // Don't process other commands while in real-time mode
        }
        
        // Normal command processing when not in real-time mode
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
        else if (input == "t") {
            bool currentMode = elevenLabsClient.isStreamingAudioEnabled();
            elevenLabsClient.enableStreamingAudio(!currentMode);
            Serial.println("Streaming audio mode: " + String(!currentMode ? "ON" : "OFF"));
        }
        else if (input == "realtime") {
            realtimeMode = true;
            Serial.println("\n[REALTIME] Mode: ENABLED");
            
            if (currentState == WAITING_FOR_TRIGGER) {
                Serial.println("[REALTIME] Starting real-time conversation mode...");
                elevenLabsClient.startRealtimeStreaming();
                if (microphone.startRealtimeStreaming(onRealtimeAudioChunk)) {
                    Serial.println("[REALTIME] ✓ Active - speak continuously for real-time conversation!");
                    Serial.println("[REALTIME] Audio will be sent in 250ms chunks");
                } else {
                    Serial.println("[REALTIME] ✗ Failed to start streaming");
                    realtimeMode = false;
                }
            } else {
                Serial.println("[REALTIME] ✗ Cannot start - not in waiting state");
                realtimeMode = false;
            }
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
            // Simple audio playback check (like Python SDK)
            if (!speaker.isPlaying()) {
                Serial.println("[RESPONSE] ✓ Response playback complete!");
                
                if (realtimeMode) {
                    // In real-time mode, just continue streaming - no state changes
                    Serial.println("[REALTIME] Continuing real-time conversation...");
                    // Stay in PLAYING_RESPONSE or go back to a listening state
                    // But don't restart recording sequences
                } else if (autoMode) {
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
    // Get raw PCM audio data from microphone
    size_t audioSize;
    int16_t* pcmData = microphone.getRawAudioData(audioSize);
    
    if (pcmData && audioSize > 0) {
        // Additional validation: ensure we have meaningful audio data
        // Check for minimum audio size (at least 1 sample = 2 bytes for 16-bit)
        if (audioSize < 2) {
            Serial.println("Audio data too small, skipping...");
            microphone.clearBuffer();
            changeState(WAITING_FOR_TRIGGER);
            return;
        }
        
        Serial.printf("Sending audio (%d bytes PCM) to ElevenLabs...\n", audioSize);
        
        // Send raw PCM audio to ElevenLabs (Python SDK style)
        elevenLabsClient.sendAudio((const uint8_t*)pcmData, audioSize);
        
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
    Serial.println("\n" + String("=").substring(0, 50));
    Serial.println("[AGENT RESPONSE] Text received:");
    Serial.println(response);
    Serial.println(String("=").substring(0, 50));
    
    // Agent response received - audio chunks will follow immediately
    Serial.println("[AGENT RESPONSE] Waiting for audio chunks...");
}

void onAudioData(const uint8_t* pcm_data, size_t size, uint32_t event_id) {
    Serial.printf("[RESPONSE] Received audio chunk (Event: %u, %d bytes PCM)\n", event_id, size);
    
    // In Python SDK style: immediately output audio to speaker (audio_interface.output)
    // No complex chunking logic needed - just play the PCM data directly
    if (speaker.isPlaying()) {
        // If speaker is busy, this could be handled by queuing or interrupting
        Serial.println("[RESPONSE] Speaker busy - implementing proper audio interface...");
    }
    
    // Convert PCM data to format expected by speaker and play immediately
    // This mimics the Python SDK's audio_interface.output(audio) behavior
    if (speaker.playPCMAudio(pcm_data, size)) {
        Serial.printf("[RESPONSE] ✓ Playing audio chunk (Event: %u, %d bytes)\n", event_id, size);
        changeState(PLAYING_RESPONSE);
    } else {
        Serial.println("[RESPONSE] ✗ Failed to play PCM audio chunk!");
        handleAudioPlaybackError();
    }
}

void onTranscript(const char* transcript) {
    Serial.println("[TRANSCRIPT] User: " + String(transcript));
}

void onInterruption(uint32_t event_id) {
    Serial.printf("[INTERRUPT] Conversation interrupted (Event ID: %u) - stopping audio playback\n", event_id);
    
    // Immediately stop audio playback (like Python SDK's audio_interface.interrupt())
    speaker.stop();
    
    // Clear any audio buffers/queues
    speaker.clearBuffer();
    
    // Return to waiting for trigger state
    changeState(WAITING_FOR_TRIGGER);
}

void onError(const char* error_message) {
    Serial.println("[ERROR] ElevenLabs Error: " + String(error_message));
    
    // Attempt to recover from errors
    if (currentState == WAITING_FOR_RESPONSE) {
        Serial.println("[ERROR] Attempting to recover...");
        delay(2000);
        changeState(WAITING_FOR_TRIGGER);
    }
}

void handleAudioPlaybackError() {
    if (autoMode) {
        // Continue conversation even if audio fails
        Serial.println("Audio failed in auto mode, continuing conversation...");
        delay(2000);
        startRecordingSequence();
    } else {
        Serial.println("Audio failed, returning to trigger wait");
        changeState(WAITING_FOR_TRIGGER);
    }
}

// Real-time streaming callback (like Python SDK input_callback)
void onRealtimeAudioChunk(const int16_t* audioData, size_t samples) {
    if (!realtimeMode || !elevenLabsClient.isConnected()) {
        return;
    }
    
    // Convert samples to bytes for transmission
    size_t audioSize = samples * sizeof(int16_t);
    const uint8_t* pcmBytes = reinterpret_cast<const uint8_t*>(audioData);
    
    // Send real-time audio chunk (like Python SDK input_callback)
    elevenLabsClient.sendRealtimeAudioChunk(pcmBytes, audioSize);
    
    // Debug output for real-time streaming
    Serial.printf("[REALTIME] Sent chunk: %d samples (%d bytes) to ElevenLabs\n", samples, audioSize);
}