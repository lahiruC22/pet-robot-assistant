#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <driver/i2s.h>
#include <esp_psram.h>
#include <Arduino.h>

/**
 * @class Microphone
 * @brief Manages I2S microphone recording functionality for INMP441 microphone.
 *
 * This class encapsulates all functionality related to recording audio from
 * an I2S microphone, managing PSRAM buffers, and encoding audio data.
 */
class Microphone {
public:
    /**
     * @brief Constructor - initializes microphone with default settings
     */
    Microphone();

    /**
     * @brief Destructor - cleans up allocated resources
     */
    ~Microphone();

    /**
     * @brief Initialize the I2S microphone with specified parameters
     * @param sampleRate Sample rate for recording (default from config.h)
     * @param bitsPerSample Bits per sample (16-bit)
     * @param bufferLen DMA buffer length
     * @return true if initialization successful, false otherwise
     */
    bool begin(uint32_t sampleRate = 16000, uint8_t bitsPerSample = 16, size_t bufferLen = 256);

    /**
     * @brief Start recording audio for specified duration
     * @param durationSeconds Duration to record in seconds
     * @return true if recording started successfully, false otherwise
     */
    bool startRecording(uint8_t durationSeconds = 5);

    /**
     * @brief Check if recording is currently in progress
     * @return true if recording, false otherwise
     */
    bool isRecording();

    /**
     * @brief Check if recording is complete and data is available
     * @return true if recording complete, false otherwise
     */
    bool isRecordingComplete();

    /**
     * @brief Get the recorded audio data as base64 encoded string
     * @return String containing base64 encoded audio data
     */
    String getBase64AudioData();

    /**
     * @brief Get the recorded audio data as raw PCM samples
     * @param dataSize Reference to store the size of returned data
     * @return Pointer to raw audio data (caller should not free this memory)
     */
    int16_t* getRawAudioData(size_t& dataSize);

    /**
     * @brief Clear the audio buffer and reset recording state
     */
    void clearBuffer();

    /**
     * @brief Get recording statistics
     * @param totalSamples Reference to store total samples recorded
     * @param totalBytes Reference to store total bytes recorded
     * @param sampleRate Reference to store current sample rate
     */
    void getRecordingStats(size_t& totalSamples, size_t& totalBytes, uint32_t& sampleRate);

    /**
     * @brief Stop current recording and cleanup
     */
    void stop();

private:
    // I2S pin configuration for INMP441
    static const int I2S_WS_PIN = 42;
    static const int I2S_SD_PIN = 41;
    static const int I2S_SCK_PIN = 1;
    static const i2s_port_t I2S_PORT = I2S_NUM_0;

    // Recording parameters
    uint32_t sampleRate;
    uint8_t bitsPerSample;
    size_t bufferLen;
    uint8_t recordingDuration;
    
    // Audio buffers
    int16_t* audioBuffer;
    int16_t* tempBuffer;
    size_t totalSamples;
    size_t totalBytes;
    size_t samplesRecorded;
    
    // State management
    bool initialized;
    bool recording;
    bool recordingComplete;
    unsigned long recordingStartTime;

    /**
     * @brief Install and configure I2S driver
     * @return true if successful, false otherwise
     */
    bool installI2S();

    /**
     * @brief Configure I2S pin connections
     * @return true if successful, false otherwise
     */
    bool configurePins();

    /**
     * @brief Allocate PSRAM buffer for audio storage
     * @return true if successful, false otherwise
     */
    bool allocateBuffers();

    /**
     * @brief Free allocated buffers
     */
    void freeBuffers();

    /**
     * @brief Internal recording loop - non-blocking
     * @return true if more recording needed, false if complete
     */
    bool recordChunk();
};

#endif
