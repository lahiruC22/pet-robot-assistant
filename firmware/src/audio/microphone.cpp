#include "microphone.h"
#include "../config.h"
#include <base64.h>

Microphone::Microphone() : 
    sampleRate(MIC_SAMPLE_RATE),
    bitsPerSample(16),
    bufferLen(256),
    recordingDuration(5),
    audioBuffer(nullptr),
    tempBuffer(nullptr),
    totalSamples(0),
    totalBytes(0),
    samplesRecorded(0),
    initialized(false),
    recording(false),
    recordingComplete(false),
    recordingStartTime(0) {
}

Microphone::~Microphone() {
    stop();
    freeBuffers();
}

bool Microphone::begin(uint32_t sampleRate, uint8_t bitsPerSample, size_t bufferLen) {
    if (initialized) {
        Serial.println("[MIC] Already initialized");
        return true;
    }

    this->sampleRate = sampleRate;
    this->bitsPerSample = bitsPerSample;
    this->bufferLen = bufferLen;

    Serial.println("[MIC] Initializing I2S microphone...");

    // Check if PSRAM is available
    if (!psramFound()) {
        Serial.println("[MIC] ERROR: PSRAM not found! This component requires PSRAM.");
        return false;
    }

    Serial.printf("[MIC] PSRAM size: %d bytes\n", ESP.getPsramSize());

    // Install I2S driver
    if (!installI2S()) {
        Serial.println("[MIC] ERROR: Failed to install I2S driver");
        return false;
    }

    // Configure pins
    if (!configurePins()) {
        Serial.println("[MIC] ERROR: Failed to configure I2S pins");
        return false;
    }

    // Start I2S
    esp_err_t err = i2s_start(I2S_PORT);
    if (err != ESP_OK) {
        Serial.printf("[MIC] ERROR: Failed to start I2S: %s\n", esp_err_to_name(err));
        return false;
    }

    // Allocate temporary buffer
    tempBuffer = (int16_t*)malloc(bufferLen * sizeof(int16_t));
    if (tempBuffer == nullptr) {
        Serial.println("[MIC] ERROR: Failed to allocate temporary buffer");
        return false;
    }

    initialized = true;
    Serial.println("[MIC] I2S microphone initialized successfully");
    return true;
}

bool Microphone::startRecording(uint8_t durationSeconds) {
    if (!initialized) {
        Serial.println("[MIC] ERROR: Microphone not initialized");
        return false;
    }

    if (recording) {
        Serial.println("[MIC] WARNING: Recording already in progress");
        return false;
    }

    this->recordingDuration = durationSeconds;
    this->totalSamples = sampleRate * recordingDuration;
    this->totalBytes = totalSamples * sizeof(int16_t);

    Serial.printf("[MIC] Starting %d second recording...\n", recordingDuration);
    Serial.printf("[MIC] Sample rate: %d Hz\n", sampleRate);
    Serial.printf("[MIC] Total samples needed: %d\n", totalSamples);
    Serial.printf("[MIC] Total bytes needed: %d\n", totalBytes);

    // Allocate PSRAM buffer for this recording
    if (!allocateBuffers()) {
        return false;
    }

    // Reset counters
    samplesRecorded = 0;
    recordingComplete = false;
    recording = true;
    recordingStartTime = millis();

    Serial.println("[MIC] Recording started!");
    return true;
}

bool Microphone::isRecording() {
    return recording;
}

bool Microphone::isRecordingComplete() {
    return recordingComplete;
}

String Microphone::getBase64AudioData() {
    if (!recordingComplete || audioBuffer == nullptr) {
        Serial.println("[MIC] WARNING: No recording data available");
        return "";
    }

    Serial.println("[MIC] Encoding audio data to base64...");
    String encodedData = base64::encode((uint8_t*)audioBuffer, totalBytes);
    
    Serial.printf("[MIC] Base64 encoding complete - Length: %d characters\n", encodedData.length());
    return encodedData;
}

int16_t* Microphone::getRawAudioData(size_t& dataSize) {
    if (!recordingComplete || audioBuffer == nullptr) {
        dataSize = 0;
        return nullptr;
    }
    
    dataSize = totalBytes;
    return audioBuffer;
}

void Microphone::clearBuffer() {
    if (recording) {
        Serial.println("[MIC] WARNING: Cannot clear buffer while recording");
        return;
    }

    freeBuffers();
    samplesRecorded = 0;
    recordingComplete = false;
    Serial.println("[MIC] Audio buffer cleared");
}

void Microphone::getRecordingStats(size_t& totalSamples, size_t& totalBytes, uint32_t& sampleRate) {
    totalSamples = this->totalSamples;
    totalBytes = this->totalBytes;
    sampleRate = this->sampleRate;
}

void Microphone::stop() {
    if (recording) {
        recording = false;
        Serial.println("[MIC] Recording stopped");
    }

    if (initialized) {
        i2s_stop(I2S_PORT);
        i2s_driver_uninstall(I2S_PORT);
        initialized = false;
        Serial.println("[MIC] I2S driver stopped");
    }
}

bool Microphone::installI2S() {
    const i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = sampleRate,
        .bits_per_sample = i2s_bits_per_sample_t(bitsPerSample),
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = 0,
        .dma_buf_count = 6,
        .dma_buf_len = bufferLen,
        .use_apll = false
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[MIC] ERROR: I2S driver install failed: %s\n", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool Microphone::configurePins() {
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = -1,
        .data_in_num = I2S_SD_PIN
    };

    esp_err_t err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[MIC] ERROR: I2S pin configuration failed: %s\n", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool Microphone::allocateBuffers() {
    // Free existing buffer if any
    freeBuffers();

    // Allocate PSRAM buffer for audio data
    audioBuffer = (int16_t*)ps_malloc(totalBytes);
    if (audioBuffer == nullptr) {
        Serial.printf("[MIC] ERROR: Failed to allocate %d bytes in PSRAM\n", totalBytes);
        return false;
    }

    Serial.printf("[MIC] Allocated %d bytes in PSRAM for audio buffer\n", totalBytes);
    return true;
}

void Microphone::freeBuffers() {
    if (audioBuffer != nullptr) {
        free(audioBuffer);
        audioBuffer = nullptr;
    }
    if (tempBuffer != nullptr) {
        free(tempBuffer);
        tempBuffer = nullptr;
    }
}

bool Microphone::recordChunk() {
    if (!recording || recordingComplete) {
        return false;
    }

    size_t bytesIn = 0;
    esp_err_t result = i2s_read(I2S_PORT, tempBuffer, bufferLen * sizeof(int16_t), &bytesIn, portMAX_DELAY);
    
    if (result == ESP_OK && bytesIn > 0) {
        size_t samplesToCopy = bytesIn / sizeof(int16_t);
        
        // Make sure we don't exceed our buffer
        if (samplesRecorded + samplesToCopy > totalSamples) {
            samplesToCopy = totalSamples - samplesRecorded;
        }
        
        // Copy samples to PSRAM buffer
        memcpy(&audioBuffer[samplesRecorded], tempBuffer, samplesToCopy * sizeof(int16_t));
        samplesRecorded += samplesToCopy;
        
        // Progress indicator every second
        if (samplesRecorded % sampleRate == 0) {
            uint8_t secondsRecorded = samplesRecorded / sampleRate;
            Serial.printf("[MIC] Recorded %d/%d seconds\n", secondsRecorded, recordingDuration);
        }
        
        // Check if recording is complete
        if (samplesRecorded >= totalSamples) {
            recording = false;
            recordingComplete = true;
            unsigned long recordingTime = millis() - recordingStartTime;
            Serial.printf("[MIC] Recording complete! Duration: %lu ms\n", recordingTime);
            Serial.printf("[MIC] Total samples recorded: %d\n", samplesRecorded);
            Serial.printf("[MIC] Total bytes recorded: %d\n", samplesRecorded * sizeof(int16_t));
            return false;
        }
    }
    
    return true; // Continue recording
}
