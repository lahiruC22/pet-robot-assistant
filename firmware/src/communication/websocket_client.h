#ifndef ELEVENLABS_CLIENT_H
#define ELEVENLABS_CLIENT_H

#include <Arduino.h>
#include <functional>
#include <WebSocketsClient.h>

// Callback function types for handling server events
using AudioDataCallback = std::function<void(const uint8_t*data, size_t length)>;
using TranscriptCallback = std::function<void(const char* transcript)>;

class ElevenLabsClient {
public:
    ElevenLabsClient();

    void begin();

    void loop();

    void sendAudio(const uint8_t* data, size_t length);

    void sendText(const char* text);

    bool isConnected();

    // -- Callbacks Registration --
    void onAudioData(AudioDataCallback callback);
    void onTranscript(TranscriptCallback callback);

private:
    friend void webSocketEventRouter(WStype_t type, uint8_t* payload, size_t length);

    void handleWebSocketMessage(uint8_t* payload, size_t length);

    void sendInitialConnectionMessage();

    // Callbacks
    AudioDataCallback audioCallback = nullptr;
    TranscriptCallback transcriptCallback = nullptr;
};

#endif