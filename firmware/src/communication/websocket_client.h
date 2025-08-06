#ifndef ELEVENLABS_CLIENT_H
#define ELEVENLABS_CLIENT_H

#include <Arduino.h>
#include <functional>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Callback function types for handling server events
using AudioDataCallback = std::function<void(const uint8_t* pcm_data, size_t size, uint32_t event_id)>;  // Raw PCM audio
using TranscriptCallback = std::function<void(const char* transcript)>;
using AgentResponseCallback = std::function<void(const char* response)>;
using ConversationInitCallback = std::function<void(const char* conversation_id)>;
using ToolCallCallback = std::function<void(const char* tool_name, const char* tool_call_id, const JsonDocument& parameters)>;
using ErrorCallback = std::function<void(const char* error_message)>;
using VadScoreCallback = std::function<void(float vad_score)>;
using PingCallback = std::function<void(uint32_t event_id, uint32_t ping_ms)>;
using ConversationEndCallback = std::function<void()>;
using InterruptionCallback = std::function<void(uint32_t event_id)>;

class ElevenLabsClient {
    
public:

    ElevenLabsClient();
    ~ElevenLabsClient();

    // Connection management
    void begin(const char* agent_id);  // Public agents - no signed URL needed
    void loop();
    void disconnect();
    bool isConnected();
    void reconnect();

    // Message sending methods  
    void sendAudio(const uint8_t* pcm_data, size_t size);  // Send raw PCM audio
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
    void onConversationEnd(ConversationEndCallback callback);
    void onInterruption(InterruptionCallback callback);  // New interrupt callback

    // Configuration methods
    void setOverrideAudio(bool override);
    void enableStreamingAudio(bool enable);
    bool isStreamingAudioEnabled();

    // Real-time streaming methods (like Python SDK input_callback)
    void startRealtimeStreaming();
    void stopRealtimeStreaming();
    bool isRealtimeStreaming();
    void sendRealtimeAudioChunk(const uint8_t* pcm_data, size_t size);

private:
    WebSocketsClient webSocket;
    WiFiClientSecure wifiClientSecure;
    static ElevenLabsClient* instance;

    // Connection parameters
    String agentId;
    String conversationId;
    bool connected;
    bool overrideAudio;
    bool streamingAudioEnabled;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectInterval;
    int reconnectAttempts;
    bool shouldReconnect;
    uint32_t lastInterruptId;  // Track interruptions like Python SDK

    // Callbacks
    AudioDataCallback audioCallback;
    TranscriptCallback transcriptCallback;
    AgentResponseCallback agentResponseCallback;
    ConversationInitCallback conversationInitCallback;
    ToolCallCallback toolCallCallback;
    ErrorCallback errorCallback;
    VadScoreCallback vadScoreCallback;
    PingCallback pingCallback;
    ConversationEndCallback conversationEndCallback;
    InterruptionCallback interruptionCallback;

    // Internal methods
    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    void handleWebSocketMessage(uint8_t* payload, size_t length);
    void sendInitialConnectionMessage();
    void processMessage(const JsonDocument& doc);
    void handleError(const char* error_message);
    void handleDisconnection();
    void resetReconnectionState();
    unsigned long getReconnectDelay();
    size_t base64Decode(const char* base64_string, uint8_t* output_buffer, size_t max_output_size);  // Base64 decoder
    String base64Encode(const uint8_t* data, size_t length);  // Base64 encoder
};

#endif