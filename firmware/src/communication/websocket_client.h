#ifndef ELEVENLABS_CLIENT_H
#define ELEVENLABS_CLIENT_H

#include <Arduino.h>
#include <functional>
#include <WebSocketsClient.h>

using MessageCallback = std::function<void(const char* type, const char* text)>;
using AudioChunkCallback = std::function<void(const char* base64Audio, int eventId, bool isComplete)>;
using ConnectionCallback = std::function<void(bool connected)>;

class ElevenLabsClient {
    
public:

    ElevenLabsClient();
    void begin(const char* agent_id);
    void loop();

    // Drop this in initial connection test phase
    //void sendAudio(const uint8_t* data, size_t length);

    void sendText(const char* text);
    void sendUserActivity();
    void sendContextualUpdate(const char* context);

    bool isConnected();
    void disconnect();
    void getConnectionInfo();

    void onMessage(MessageCallback callback);
    void onAudioChunk(AudioChunkCallback callback);
    void onConnection(ConnectionCallback callback);

    // Audio chunk collection methods
    void clearAudioBuffer();
    String getCompleteAudioBase64();
    int getAudioChunkCount();

private:

    WebSocketsClient WebSocket;
    String agentId;
    
    bool connected = false;
    bool conversationInitiated = false;
    unsigned long connectionTime = 0;
    int messagesSent = 0;
    int messagesReceived = 0;

    // Audio chunk collection
    String audioBuffer;
    int audioChunkCount = 0;
    int lastAudioEventId = -1;
    bool audioStreamActive = false;

    MessageCallback messageCallback = nullptr;
    AudioChunkCallback audioChunkCallback = nullptr;
    ConnectionCallback connectionCallback = nullptr;

    void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    static void webSocketEventRouter(WStype_t type, uint8_t* payload, size_t length);

};

#endif