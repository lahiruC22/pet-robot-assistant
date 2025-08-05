#ifndef SPEAKER_H
#define SPEAKER_H

#include <driver/i2s.h>
#include <Arduino.h>

/**
 * @class Speaker
 * @brief Manages I2S speaker playback functionality for audio output.
 *
 * This class encapsulates all functionality related to playing audio through
 * an I2S DAC/amplifier, managing audio buffers, and handling real-time playback.
 * Optimized for ElevenLabs Conversational AI audio responses.
 */
class Speaker {
public:
    /**
     * @brief Constructor - initializes speaker with default settings
     */
    Speaker();

    /**
     * @brief Destructor - cleans up allocated resources
     */
    ~Speaker();

    /**
     * @brief Initialize the I2S speaker with specified parameters
     * @param sampleRate Sample rate for playback (default from config.h: 24000Hz)
     * @param bitsPerSample Bits per sample (16-bit)
     * @param bufferLen DMA buffer length for playback
     * @return true if initialization successful, false otherwise
     */
    bool begin(uint32_t sampleRate = 24000, uint8_t bitsPerSample = 16, int bufferLen = 1024);

    /**
     * @brief Play audio data from base64 encoded string (ElevenLabs format)
     * @param base64AudioData Base64 encoded PCM audio data
     * @return true if playback started successfully, false otherwise
     */
    bool playBase64Audio(const String& base64AudioData);

    /**
     * @brief Play raw PCM audio data
     * @param audioData Pointer to PCM audio samples
     * @param audioSize Size of audio data in bytes
     * @return true if playback started successfully, false otherwise
     */
    bool playRawAudio(const int16_t* audioData, size_t audioSize);

    /**
     * @brief Check if audio is currently playing
     * @return true if playing, false otherwise
     */
    bool isPlaying();

    /**
     * @brief Stop current audio playback
     */
    void stop();

    /**
     * @brief Set audio volume (0.0 to 1.0)
     * @param volume Volume level (0.0 = mute, 1.0 = full volume)
     */
    void setVolume(float volume);

    /**
     * @brief Get current volume level
     * @return Current volume (0.0 to 1.0)
     */
    float getVolume();

    /**
     * @brief Main loop function for audio playback management
     * Call this repeatedly in the main loop when playing audio
     */
    void loop();

    /**
     * @brief Clear any queued audio and reset playback state
     */
    void clearBuffer();

    /**
     * @brief Get playback statistics
     * @param totalSamples Reference to store total samples played
     * @param currentPosition Reference to store current playback position
     * @param sampleRate Reference to store current sample rate
     */
    void getPlaybackStats(size_t& totalSamples, size_t& currentPosition, uint32_t& sampleRate);

private:
    // I2S pin configuration for audio output
    static const int I2S_WS_PIN = 45;   // LRC (Left/Right Clock)
    static const int I2S_SD_PIN = 48;   // DIN (Data Input)
    static const int I2S_SCK_PIN = 47;  // BCLK (Bit Clock)
    static const i2s_port_t I2S_PORT = I2S_NUM_1;  // Use port 1 (port 0 used by microphone)

    // Audio parameters
    uint32_t sampleRate;
    uint8_t bitsPerSample;
    int bufferLen;
    float volume;
    
    // Audio buffers
    int16_t* audioBuffer;
    size_t audioBufferSize;
    size_t audioSamples;
    size_t playbackPosition;
    
    // State management
    bool initialized;
    bool playing;
    unsigned long playbackStartTime;

    /**
     * @brief Install and configure I2S driver for output
     * @return true if successful, false otherwise
     */
    bool installI2S();

    /**
     * @brief Configure I2S pin connections for output
     * @return true if successful, false otherwise
     */
    bool configurePins();

    /**
     * @brief Internal playback loop - non-blocking
     * @return true if more playback needed, false if complete
     */
    bool playbackChunk();

    /**
     * @brief Decode base64 audio data to PCM samples
     * @param base64Data Base64 encoded audio string
     * @param decodedSize Reference to store decoded data size
     * @return Pointer to decoded PCM data
     * 
     * @warning MEMORY OWNERSHIP: The returned pointer is allocated with malloc()
     *          and MUST be freed by the caller using free(). Failure to do so
     *          will result in memory leaks.
     * 
     * @note The caller is responsible for:
     *       1. Checking if the returned pointer is not nullptr
     *       2. Using the data before freeing it
     *       3. Calling free() on the returned pointer when done
     * 
     * @example
     *   size_t size;
     *   int16_t* data = decodeBase64Audio(base64String, size);
     *   if (data != nullptr) {
     *       // Use the data...
     *       free(data);  // Must free when done
     *   }
     */
    int16_t* decodeBase64Audio(const String& base64Data, size_t& decodedSize);

    /**
     * @brief Apply volume adjustment to audio samples
     * @param samples Pointer to audio samples
     * @param sampleCount Number of samples to process
     */
    void applyVolume(int16_t* samples, size_t sampleCount);

    /**
     * @brief Free allocated audio buffer
     */
    void freeAudioBuffer();
};

#endif
