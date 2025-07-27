#ifndef ELEVENLABS_CLIENT_H
#define ELEVENLABS_CLIENT_H

#include <Arduino.h>
#include <functional>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Callback function types for handling server events
using AudioDataCallback = std::function<void(const uint8_t* data, size_t length, uint32_t event_id)>;
using TranscriptCallback = std::function<void(const char* transcript)>;
using AgentResponseCallback = std::function<void(const char* response)>;
using ConversationInitCallback = std::function<void(const char* conversation_id)>;
using ToolCallCallback = std::function<void(const char* tool_name, const char* tool_call_id, const JsonDocument& parameters)>;
using ErrorCallback = std::function<void(const char* error_message)>;
using VadScoreCallback = std::function<void(float vad_score)>;
using PingCallback = std::function<void(uint32_t event_id, uint32_t ping_ms)>;

class ElevenLabsClient {
public:
    ElevenLabsClient();
    ~ElevenLabsClient();

    // Connection management
    void begin(const char* agent_id);
    void loop();
    void disconnect();
    bool isConnected();
    void reconnect();

    // Message sending methods
    void sendAudio(const uint8_t* data, size_t length);
    void sendText(const char* text);
    void sendUserActivity();
    void sendContextualUpdate(const char* text);
    void sendToolResult(const char* tool_call_id, const char* result, bool is_error = false);
    void sendPong(uint32_t event_id);

    // Callback registration methods
    void onAudioData(AudioDataCallback callback);
    void onTranscript(TranscriptCallback callback);
    void onAgentResponse(AgentResponseCallback callback);
    void onConversationInit(ConversationInitCallback callback);
    void onToolCall(ToolCallCallback callback);
    void onError(ErrorCallback callback);
    void onVadScore(VadScoreCallback callback);
    void onPing(PingCallback callback);

    // Configuration methods
    void setOverrideAudio(bool override);

private:
    WebSocketsClient webSocket;
    WiFiClientSecure wifiClientSecure;
    static ElevenLabsClient* instance;

    // Connection parameters
    String agentId;
    String conversationId;
    bool connected;
    bool overrideAudio;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectInterval;
    int reconnectAttempts;
    bool shouldReconnect;

    // Callbacks
    AudioDataCallback audioCallback;
    TranscriptCallback transcriptCallback;
    AgentResponseCallback agentResponseCallback;
    ConversationInitCallback conversationInitCallback;
    ToolCallCallback toolCallCallback;
    ErrorCallback errorCallback;
    VadScoreCallback vadScoreCallback;
    PingCallback pingCallback;

    // Internal methods
    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    void handleWebSocketMessage(uint8_t* payload, size_t length);
    void sendInitialConnectionMessage();
    void processMessage(const JsonDocument& doc);
    void handleError(const char* error_message);
    void handleDisconnection();
    void resetReconnectionState();
    unsigned long getReconnectDelay();
};

#endif