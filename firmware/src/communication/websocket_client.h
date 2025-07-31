#ifndef ELEVENLABS_CLIENT_H
#define ELEVENLABS_CLIENT_H

#include <Arduino.h>
#include <functional>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

<<<<<<< HEAD
using MessageCallback = std::function<void(const char* type, const char* text)>;
using AudioChunkCallback = std::function<void(const char* base64Audio, int eventId, bool isComplete)>;
using ConnectionCallback = std::function<void(bool connected)>;
=======
// Callback function types for handling server events
using AudioDataCallback = std::function<void(const uint8_t* data, size_t length, uint32_t event_id)>;
using TranscriptCallback = std::function<void(const char* transcript)>;
using AgentResponseCallback = std::function<void(const char* response)>;
using ConversationInitCallback = std::function<void(const char* conversation_id)>;
using ToolCallCallback = std::function<void(const char* tool_name, const char* tool_call_id, const JsonDocument& parameters)>;
using ErrorCallback = std::function<void(const char* error_message)>;
using VadScoreCallback = std::function<void(float vad_score)>;
using PingCallback = std::function<void(uint32_t event_id, uint32_t ping_ms)>;
>>>>>>> upstream/feature/WIZ-53-websocket-connection

class ElevenLabsClient {
    
public:

    ElevenLabsClient();
<<<<<<< HEAD
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

=======
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
>>>>>>> upstream/feature/WIZ-53-websocket-connection
};

#endif