/*
  ESP32 S3 I2S Microphone Testing Application
  Tests the Microphone class functionality
  Records 5 seconds of audio to PSRAM and outputs as base64
  Requires INMP441 I2S microphone
*/

#include <Arduino.h>
#include "../config.h"
#include "microphone.h"

// Create microphone instance
Microphone microphone;

void countdown() {
  Serial.println("\n🎤 Starting recording countdown...");
  for (int i = 5; i >= 1; i--) {
    Serial.printf("Recording starts in: %d\n", i);
    delay(1000);
  }
  Serial.println("Recording started!\n");
}

void displayRecordingStats() {
  size_t totalSamples, totalBytes;
  uint32_t sampleRate;
  
  microphone.getRecordingStats(totalSamples, totalBytes, sampleRate);
  
  Serial.println("\nRECORDING STATISTICS:");
  Serial.printf("Sample Rate: %d Hz\n", sampleRate);
  Serial.printf("Total Samples: %d\n", totalSamples);
  Serial.printf("Total Bytes: %d\n", totalBytes);
  Serial.printf("Duration: %.1f seconds\n", (float)totalSamples / sampleRate);
}

void displayBase64Audio() {
  String encodedData = microphone.getBase64AudioData();
  
  if (encodedData.length() > 0) {
    Serial.println("\n" + String("=").substring(0, 60));
    Serial.println("BASE64 ENCODED AUDIO DATA:");
    Serial.println(String("=").substring(0, 60));
    Serial.println(encodedData);
    Serial.println(String("=").substring(0, 60));
    Serial.printf("Base64 string length: %d characters\n", encodedData.length());
    Serial.printf("Estimated decoded size: %d bytes\n", (encodedData.length() * 3) / 4);
  } else {
    Serial.println("Failed to get base64 audio data");
  }
}

void setup() {
  // Set up Serial Monitor
  Serial.begin(115200);
  Serial.println("\nESP32 Pet Robot Assistant - Microphone Test");
  Serial.println("================================================");
  
  // Initialize microphone with config settings
  if (!microphone.begin(MIC_SAMPLE_RATE)) {
    Serial.println("Failed to initialize microphone!");
    Serial.println("Check PSRAM availability and I2S connections");
    while (1) delay(1000);
  }
  
  Serial.println("Microphone initialized successfully");
  delay(1000);
  
  // Start the recording process
  countdown();
  
  if (!microphone.startRecording(5)) {
    Serial.println("Failed to start recording!");
    while (1) delay(1000);
  }
}

void loop() {
  // Check if microphone is still recording
  if (microphone.isRecording()) {
    // Non-blocking: could do other tasks here
    delay(100);
  } 
  // Check if recording just completed
  else if (microphone.isRecordingComplete()) {
    Serial.println("\nRecording completed successfully!");
    
    // Display recording statistics
    displayRecordingStats();
    
    // Display base64 encoded audio
    displayBase64Audio();
    
    // Clear buffer to prepare for next recording
    microphone.clearBuffer();
    
    Serial.println("\nTest completed successfully!");
    Serial.println("You can reset the device to run the test again");
    
    // Wait indefinitely
    while (true) {
      delay(10000);
      Serial.println("Reset device to run microphone test again");
    }
  }
  // Handle any error states
  else {
    delay(100);
  }
}