#ifndef ELEVENLABS_CLIENT_H
#define ELEVENLABS_CLIENT_H

#include <Arduino.h>
#include <functional>
#include <WebSocketsClient.h>

using MessageCallback = std::function<void(const char* type, const char* text)>;
using AudioDataCallback = std::function<void(const uint8_t*data, size_t length)>;
using ConnectionCallback = std::function<void(bool connected)>;

class ElevenLabsClient {
    
public:

    ElevenLabsClient();
    void begin(const char* agent_id, const char* api_key);
    void loop();

    // Drop this in initial connection test phase
    //void sendAudio(const uint8_t* data, size_t length);

    void sendText(const char* text);

    bool isConnected();

    void onMessage(MessageCallback callback);
    void onAudioData(AudioDataCallback callback);
    void onConnection(ConnectionCallback callback);

private:

    WebSocketsClient WebSocket;
    String agentId;
    String apikey;
    bool connected = false;

    MessageCallback messageCallback = nullptr;
    AudioDataCallback audioDataCallback = nullptr;
    ConnectionCallback connectionCallback = nullptr;

    void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    static void webSocketEventRouter(WStype_t type, uint8_t* payload, size_t length);

};

#endif