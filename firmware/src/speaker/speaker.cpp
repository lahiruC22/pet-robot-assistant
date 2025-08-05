#include "speaker.h"
#include "../config.h"
#include "mbedtls/base64.h"

Speaker::Speaker() : 
    sampleRate(SPEAKER_SAMPLE_RATE),
    bitsPerSample(16),
    bufferLen(1024),
    volume(0.7f),  // Default to 70% volume
    audioBuffer(nullptr),
    audioBufferSize(0),
    audioSamples(0),
    playbackPosition(0),
    stereoBuffer(nullptr),
    stereoBufferSize(0),
    initialized(false),
    playing(false),
    playbackStartTime(0) {
}

Speaker::~Speaker() {
    stop();
    
    // Properly uninitialize I2S driver if still initialized
    if (initialized) {
        i2s_stop(I2S_PORT);
        i2s_driver_uninstall(I2S_PORT);
        initialized = false;
        Serial.println("[SPEAKER] I2S driver uninstalled in destructor");
    }
    
    freeAudioBuffer();
    freeStereoBuffer();
}

bool Speaker::begin(uint32_t sampleRate, uint8_t bitsPerSample, int bufferLen) {
    if (initialized) {
        Serial.println("[SPEAKER] Already initialized");
        return true;
    }

    this->sampleRate = sampleRate;
    this->bitsPerSample = bitsPerSample;
    this->bufferLen = bufferLen;

    Serial.println("[SPEAKER] Initializing I2S speaker...");
    Serial.printf("[SPEAKER] Sample rate: %d Hz\n", sampleRate);
    Serial.printf("[SPEAKER] Bits per sample: %d\n", bitsPerSample);
    Serial.printf("[SPEAKER] Buffer length: %d\n", bufferLen);

    // Install I2S driver
    if (!installI2S()) {
        Serial.println("[SPEAKER] ERROR: Failed to install I2S driver");
        return false;
    }

    // Configure pins
    if (!configurePins()) {
        Serial.println("[SPEAKER] ERROR: Failed to configure I2S pins");
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }

    // Start I2S
    esp_err_t err = i2s_start(I2S_PORT);
    if (err != ESP_OK) {
        Serial.printf("[SPEAKER] ERROR: Failed to start I2S: %s\n", esp_err_to_name(err));
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }

    // Allocate stereo buffer for mono-to-stereo conversion
    // Size it for the maximum chunk size (bufferLen samples * 2 for stereo)
    if (!allocateStereoBuffer(bufferLen * 2)) {
        Serial.println("[SPEAKER] ERROR: Failed to allocate stereo buffer");
        i2s_stop(I2S_PORT);
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }

    initialized = true;
    Serial.println("[SPEAKER] I2S speaker initialized successfully");
    return true;
}

bool Speaker::playBase64Audio(const String& base64AudioData) {
    if (!initialized) {
        Serial.println("[SPEAKER] ERROR: Speaker not initialized");
        return false;
    }

    if (playing) {
        Serial.println("[SPEAKER] WARNING: Already playing audio, stopping current playback");
        stop();
    }

    if (base64AudioData.length() == 0) {
        Serial.println("[SPEAKER] ERROR: No audio data provided");
        return false;
    }

    Serial.printf("[SPEAKER] Decoding base64 audio: %d characters\n", base64AudioData.length());

    // Decode base64 audio data
    size_t decodedSize = 0;
    int16_t* decodedAudio = decodeBase64Audio(base64AudioData, decodedSize);
    
    if (decodedAudio == nullptr || decodedSize == 0) {
        Serial.println("[SPEAKER] ERROR: Failed to decode base64 audio");
        return false;
    }

    // Store audio data for playback
    freeAudioBuffer();  // Free any existing buffer
    audioBuffer = decodedAudio;
    audioBufferSize = decodedSize;
    audioSamples = decodedSize / sizeof(int16_t);
    playbackPosition = 0;

    // Apply volume adjustment
    applyVolume(audioBuffer, audioSamples);

    Serial.printf("[SPEAKER] Audio ready for playback: %d samples, %d bytes\n", audioSamples, audioBufferSize);
    Serial.printf("[SPEAKER] Duration: %.2f seconds\n", (float)audioSamples / sampleRate);

    // Start playback
    playing = true;
    playbackStartTime = millis();

    Serial.println("[SPEAKER] Audio playback started");
    return true;
}

bool Speaker::playRawAudio(const int16_t* audioData, size_t audioSize) {
    if (!initialized) {
        Serial.println("[SPEAKER] ERROR: Speaker not initialized");
        return false;
    }

    if (playing) {
        Serial.println("[SPEAKER] WARNING: Already playing audio, stopping current playback");
        stop();
    }

    if (audioData == nullptr || audioSize == 0) {
        Serial.println("[SPEAKER] ERROR: No audio data provided");
        return false;
    }

    // Allocate and copy audio data
    freeAudioBuffer();
    audioBuffer = (int16_t*)malloc(audioSize);
    if (audioBuffer == nullptr) {
        #ifdef ESP_PLATFORM
        Serial.printf("[SPEAKER] ERROR: Failed to allocate audio buffer. Requested size: %u bytes, Free heap: %u bytes\n", (unsigned int)audioSize, (unsigned int)ESP.getFreeHeap());
        #else
        Serial.printf("[SPEAKER] ERROR: Failed to allocate audio buffer. Requested size: %u bytes\n", (unsigned int)audioSize);
        #endif
        return false;
    }

    memcpy(audioBuffer, audioData, audioSize);
    audioBufferSize = audioSize;
    audioSamples = audioSize / sizeof(int16_t);
    playbackPosition = 0;

    // Apply volume adjustment
    applyVolume(audioBuffer, audioSamples);

    Serial.printf("[SPEAKER] Raw audio ready for playback: %d samples, %d bytes\n", audioSamples, audioBufferSize);

    // Start playback
    playing = true;
    playbackStartTime = millis();

    Serial.println("[SPEAKER] Raw audio playback started");
    return true;
}

bool Speaker::isPlaying() {
    return playing;
}

void Speaker::stop() {
    if (playing) {
        playing = false;
        playbackPosition = 0;
        
        // Stop I2S transmission without uninstalling driver
        if (initialized) {
            i2s_stop(I2S_PORT);
            i2s_start(I2S_PORT);  // Restart to clear any pending data
        }
        
        Serial.println("[SPEAKER] Audio playback stopped");
    }
    // Remove the hardware deinitialization from stop()
    // Hardware should only be deinitialized in destructor
}

void Speaker::setVolume(float volume) {
    this->volume = constrain(volume, 0.0f, 1.0f);
    Serial.printf("[SPEAKER] Volume set to: %.2f\n", this->volume);
}

float Speaker::getVolume() {
    return volume;
}

void Speaker::loop() {
    if (playing && initialized) {
        if (!playbackChunk()) {
            // Playback completed
            playing = false;
            unsigned long playbackDuration = millis() - playbackStartTime;
            Serial.printf("[SPEAKER] Playback completed in %lu ms\n", playbackDuration);
        }
    }
}

void Speaker::clearBuffer() {
    if (playing) {
        stop();
    }
    freeAudioBuffer();
    playbackPosition = 0;
    Serial.println("[SPEAKER] Audio buffer cleared");
}

void Speaker::getPlaybackStats(size_t& totalSamples, size_t& currentPosition, uint32_t& sampleRate) {
    totalSamples = this->audioSamples;
    currentPosition = this->playbackPosition;
    sampleRate = this->sampleRate;
}

bool Speaker::installI2S() {
    const i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = sampleRate,
        .bits_per_sample = i2s_bits_per_sample_t(bitsPerSample),
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // Better DAC compatibility
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = 0,
        .dma_buf_count = 6,
        .dma_buf_len = bufferLen,
        .use_apll = false
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[SPEAKER] ERROR: I2S driver install failed: %s\n", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool Speaker::configurePins() {
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_SD_PIN,
        .data_in_num = -1  // Not used for output
    };

    esp_err_t err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[SPEAKER] ERROR: I2S pin configuration failed: %s\n", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool Speaker::playbackChunk() {
    if (!playing || audioBuffer == nullptr || playbackPosition >= audioSamples) {
        return false;  // Playback complete
    }

    // Calculate how many samples to write in this chunk
    size_t monoSamplesToWrite = min((size_t)bufferLen, audioSamples - playbackPosition);
    
    // Since we're using I2S_CHANNEL_FMT_RIGHT_LEFT, we need to duplicate mono samples to stereo
    // Use pre-allocated stereo buffer to avoid memory fragmentation
    size_t stereoSamples = monoSamplesToWrite * 2;
    
    // Ensure our pre-allocated buffer is large enough
    if (stereoBuffer == nullptr || stereoSamples * sizeof(int16_t) > stereoBufferSize) {
        Serial.println("[SPEAKER] ERROR: Stereo buffer too small or not allocated");
        return false;
    }
    
    // Duplicate mono samples to stereo (L and R channels get same data)
    for (size_t i = 0; i < monoSamplesToWrite; i++) {
        stereoBuffer[i * 2] = audioBuffer[playbackPosition + i];     // Left channel
        stereoBuffer[i * 2 + 1] = audioBuffer[playbackPosition + i]; // Right channel
    }
    
    size_t bytesToWrite = stereoSamples * sizeof(int16_t);
    size_t bytesWritten = 0;
    esp_err_t result = i2s_write(I2S_PORT, 
                                stereoBuffer, 
                                bytesToWrite, 
                                &bytesWritten, 
                                portMAX_DELAY);
    
    if (result == ESP_OK && bytesWritten > 0) {
        size_t stereoSamplesWritten = bytesWritten / sizeof(int16_t);
        size_t monoSamplesWritten = stereoSamplesWritten / 2;  // Convert back to mono count
        playbackPosition += monoSamplesWritten;

        // Progress indicator (every second)
        if ((playbackPosition % sampleRate) < monoSamplesWritten) {
            float secondsPlayed = (float)playbackPosition / sampleRate;
            float totalSeconds = (float)audioSamples / sampleRate;
            Serial.printf("[SPEAKER] Playing: %.1f/%.1f seconds\n", secondsPlayed, totalSeconds);
        }

        // Check if playback is complete
        if (playbackPosition >= audioSamples) {
            Serial.println("[SPEAKER] Audio playback finished");
            return false;
        }
    } else {
        Serial.printf("[SPEAKER] ERROR: I2S write failed: %s\n", esp_err_to_name(result));
        return false;
    }

    return true;  // Continue playback
}

int16_t* Speaker::decodeBase64Audio(const String& base64Data, size_t& decodedSize) {
    // Calculate required buffer size for base64 decoding
    size_t requiredLen = 0;
    int result = mbedtls_base64_decode(NULL, 0, &requiredLen, 
                                      (const unsigned char*)base64Data.c_str(), base64Data.length());
    
    if (result != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
        Serial.println("[SPEAKER] ERROR: Invalid base64 data");
        decodedSize = 0;
        return nullptr;
    }
    
    // Allocate buffer for decoded data
    uint8_t* decodedBytes = (uint8_t*)malloc(requiredLen);
    if (decodedBytes == nullptr) {
        Serial.println("[SPEAKER] ERROR: Failed to allocate decode buffer");
        decodedSize = 0;
        return nullptr;
    }

    // Perform base64 decoding
    result = mbedtls_base64_decode(decodedBytes, requiredLen, &decodedSize,
                                  (const unsigned char*)base64Data.c_str(), base64Data.length());
    
    if (result != 0) {
        Serial.println("[SPEAKER] ERROR: Base64 decode failed");
        free(decodedBytes);
        decodedSize = 0;
        return nullptr;
    }

    Serial.printf("[SPEAKER] Base64 decode successful: %d bytes\n", decodedSize);
    
    // Return as int16_t pointer (PCM audio data)
    return (int16_t*)decodedBytes;
}

void Speaker::applyVolume(int16_t* samples, size_t sampleCount) {
    if (volume == 1.0f) {
        return;  // No volume adjustment needed
    }

    for (size_t i = 0; i < sampleCount; i++) {
        int32_t adjustedSample = (int32_t)(samples[i] * volume);
        
        // Clamp to int16_t range
        if (adjustedSample > INT16_MAX) {
            adjustedSample = INT16_MAX;
        } else if (adjustedSample < INT16_MIN) {
            adjustedSample = INT16_MIN;
        }
        
        samples[i] = (int16_t)adjustedSample;
    }
}

void Speaker::freeAudioBuffer() {
    if (audioBuffer != nullptr) {
        free(audioBuffer);
        audioBuffer = nullptr;
        audioBufferSize = 0;
        audioSamples = 0;
    }
}

bool Speaker::allocateStereoBuffer(size_t maxStereoSamples) {
    if (stereoBuffer != nullptr) {
        return true; // Already allocated
    }
    
    stereoBufferSize = maxStereoSamples * sizeof(int16_t);
    stereoBuffer = (int16_t*)malloc(stereoBufferSize);
    if (stereoBuffer == nullptr) {
        Serial.println("Failed to allocate stereo buffer");
        stereoBufferSize = 0;
        return false;
    }
    
    return true;
}

void Speaker::freeStereoBuffer() {
    if (stereoBuffer != nullptr) {
        free(stereoBuffer);
        stereoBuffer = nullptr;
        stereoBufferSize = 0;
    }
}
