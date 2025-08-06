#include "websocket_client.h"
#include "../config.h"

// Speaker audio configuration
// #define SPEAKER_BYTES_PER_SAMPLE 2  // 16-bit PCM audio = 2 bytes per sample
// #define SPEAKER_SAMPLE_RATE 16000   // Set your speaker sample rate (e.g., 16000 Hz)

// ElevenLabs WebSocket server certificate (root CA)
const char* elevenlabs_ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"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\n" \
"-----END CERTIFICATE-----\n";

// Static instance for callback handling
ElevenLabsClient* ElevenLabsClient::instance = nullptr;

ElevenLabsClient::ElevenLabsClient() : 
    connected(false),
    overrideAudio(true),
    streamingAudioEnabled(true),
    lastReconnectAttempt(0),
    reconnectInterval(5000),
    reconnectAttempts(0),
    shouldReconnect(false),
    lastInterruptId(0),  // Initialize interrupt tracking
    audioCallback(nullptr),
    transcriptCallback(nullptr),
    agentResponseCallback(nullptr),
    conversationInitCallback(nullptr),
    toolCallCallback(nullptr),
    errorCallback(nullptr),
    vadScoreCallback(nullptr),
    pingCallback(nullptr),
    conversationEndCallback(nullptr),
    interruptionCallback(nullptr) {
    instance = this;
}

ElevenLabsClient::~ElevenLabsClient() {
    disconnect();
    instance = nullptr;
}

void ElevenLabsClient::begin(const char* agent_id) {
    agentId = String(agent_id);
    shouldReconnect = true;
    
    Serial.println("Initializing ElevenLabs WebSocket connection...");
    
    // Configure SSL client
    wifiClientSecure.setCACert(elevenlabs_ca_cert);
    wifiClientSecure.setTimeout(10000);
    
    // Direct connection to public agent endpoint
    String wsUrl = "/v1/convai/conversation?agent_id=" + agentId;
    Serial.println("Connecting to: api.elevenlabs.io" + wsUrl);
    webSocket.beginSSL("api.elevenlabs.io", 443, wsUrl.c_str(), "", "https");
    
    webSocket.onEvent(webSocketEvent);
    
    // Configure heartbeat - more conservative settings to prevent disconnections
    webSocket.enableHeartbeat(30000, 5000, 3);  // 30s ping interval, 5s timeout, 3 retries
    
    // Set reconnect interval
    webSocket.setReconnectInterval(5000);
    
    // Reset connection state
    resetReconnectionState();
    
    Serial.println("WebSocket configured with SSL, attempting connection...");
}

void ElevenLabsClient::loop() {
    webSocket.loop();
    
    // Handle reconnection if needed
    if (!connected && shouldReconnect) {
        unsigned long currentTime = millis();
        if (currentTime - lastReconnectAttempt >= getReconnectDelay()) {
            Serial.println("Attempting to reconnect to ElevenLabs WebSocket...");
            reconnect();
            lastReconnectAttempt = currentTime;
        }
    }
}

void ElevenLabsClient::disconnect() {
    if (connected) {
        shouldReconnect = false;
        webSocket.disconnect();
        connected = false;
        Serial.println("WebSocket disconnected");
    }
}

bool ElevenLabsClient::isConnected() {
    return connected && (WiFi.status() == WL_CONNECTED);
}

void ElevenLabsClient::reconnect() {
    if (!agentId.isEmpty() && WiFi.status() == WL_CONNECTED) {
        reconnectAttempts++;
        Serial.printf("WebSocket reconnection attempt #%d\n", reconnectAttempts);
        
        // Disconnect first if still connected
        webSocket.disconnect();
        delay(1000);
        
        // Reconfigure SSL and reconnect
        wifiClientSecure.setCACert(elevenlabs_ca_cert);
        
        // Direct connection to public agent endpoint
        String wsUrl = "/v1/convai/conversation?agent_id=" + agentId;
        webSocket.beginSSL("api.elevenlabs.io", 443, wsUrl.c_str(), "", "https");
    } else {
        Serial.println("Cannot reconnect: WiFi not connected or agent ID missing");
    }
}

void ElevenLabsClient::sendAudio(const uint8_t* pcm_data, size_t size) {
    if (!connected) {
        handleError("Cannot send audio: WebSocket not connected");
        return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        handleError("Cannot send audio: WiFi not connected");
        return;
    }
    
    if (!pcm_data || size == 0) {
        handleError("Cannot send audio: No audio data provided");
        return;
    }
    
    // CRITICAL FIX: Split large audio into smaller chunks
    // ElevenLabs Python SDK sends ~250ms chunks (4000 samples = 8000 bytes at 16kHz)
    const size_t MAX_CHUNK_SIZE = 8000; // 250ms at 16kHz (like Python SDK)
    
    size_t offset = 0;
    int chunkCount = 0;
    
    while (offset < size) {
        size_t chunkSize = min(MAX_CHUNK_SIZE, size - offset);
        
        // Encode this chunk to base64
        String base64Audio = base64Encode(pcm_data + offset, chunkSize);
        
        // Validate base64 string is not empty
        if (base64Audio.length() == 0) {
            handleError("Cannot send audio: Base64 encoding failed for chunk");
            return;
        }
        
        JsonDocument doc;
        doc["user_audio_chunk"] = base64Audio;
        
        String message;
        serializeJson(doc, message);
        
        bool success = webSocket.sendTXT(message);
        if (success) {
            chunkCount++;
            Serial.printf("Sent audio chunk %d: %d bytes PCM -> %d chars base64\n", 
                         chunkCount, chunkSize, base64Audio.length());
        } else {
            handleError("Failed to send audio chunk");
            return;
        }
        
        offset += chunkSize;
        
        // Small delay between chunks to avoid overwhelming the server
        delay(10);
    }
    
    Serial.printf("Audio transmission complete: %d total chunks\n", chunkCount);
}

void ElevenLabsClient::sendText(const char* text) {
    if (!connected) {
        handleError("Cannot send text: WebSocket not connected");
        return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        handleError("Cannot send text: WiFi not connected");
        return;
    }
    
    JsonDocument doc;
    doc["type"] = "user_message";
    doc["text"] = text;
    
    String message;
    serializeJson(doc, message);
    
    bool success = webSocket.sendTXT(message);
    if (success) {
        Serial.println("Sent text message: " + String(text));
    } else {
        handleError("Failed to send text message");
    }
}

void ElevenLabsClient::sendUserActivity() {
    if (!connected) {
        handleError("Cannot send user activity: WebSocket not connected");
        return;
    }
    
    JsonDocument doc;
    doc["type"] = "user_activity";
    
    String message;
    serializeJson(doc, message);
    
    webSocket.sendTXT(message);
}

void ElevenLabsClient::sendContextualUpdate(const char* text) {
    if (!connected) {
        handleError("Cannot send contextual update: WebSocket not connected");
        return;
    }
    
    JsonDocument doc;
    doc["type"] = "contextual_update";
    doc["text"] = text;
    
    String message;
    serializeJson(doc, message);
    
    webSocket.sendTXT(message);
}

void ElevenLabsClient::sendToolResult(const char* tool_call_id, const char* result, bool is_error) {
    if (!connected) {
        handleError("Cannot send tool result: WebSocket not connected");
        return;
    }
    
    JsonDocument doc;
    doc["type"] = "client_tool_result";
    doc["tool_call_id"] = tool_call_id;
    doc["result"] = result;
    doc["is_error"] = is_error;
    
    String message;
    serializeJson(doc, message);
    
    webSocket.sendTXT(message);
}

void ElevenLabsClient::sendPong(uint32_t event_id) {
    if (!connected) {
        handleError("Cannot send pong: WebSocket not connected");
        return;
    }
    
    JsonDocument doc;
    doc["type"] = "pong";
    doc["event_id"] = event_id;
    
    String message;
    serializeJson(doc, message);
    
    webSocket.sendTXT(message);
    Serial.printf("Sent pong for event ID: %u\n", event_id);
}

void ElevenLabsClient::sendInitialConnectionMessage() {
    if (!connected) {
        Serial.println("Cannot send initial message: not connected");
        return;
    }
    
    JsonDocument doc;
    doc["type"] = "conversation_initiation_client_data";
    
    // Audio configuration - override default audio handling
    if (overrideAudio) {
        doc["conversation_config_override"]["override_agent_output_audio"] = true;
    }
    
    String message;
    serializeJson(doc, message);
    
    bool success = webSocket.sendTXT(message);
    if (success) {
        Serial.println("Sent initial connection message");
    } else {
        Serial.println("Failed to send initial connection message");
        handleError("Failed to send initial connection message");
    }
}

void ElevenLabsClient::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (instance) {
        switch(type) {
            case WStype_DISCONNECTED:
                Serial.printf("WebSocket Disconnected - Reason: %s\n", payload ? (char*)payload : "Unknown");
                instance->handleDisconnection();
                break;
                
            case WStype_CONNECTED:
                Serial.printf("WebSocket Connected to: %s\n", payload);
                instance->connected = true;
                instance->resetReconnectionState();
                instance->sendInitialConnectionMessage();
                break;
                
            case WStype_TEXT:
                Serial.printf("Received text: %s\n", payload);
                instance->handleWebSocketMessage(payload, length);
                break;
                
            case WStype_BIN:
                Serial.printf("Received binary data: %u bytes\n", length);
                break;
                
            case WStype_ERROR:
                Serial.printf("WebSocket Error: %s\n", payload ? (char*)payload : "Unknown error");
                instance->handleError(payload ? (char*)payload : "Unknown WebSocket error");
                break;
                
            case WStype_FRAGMENT_TEXT_START:
            case WStype_FRAGMENT_BIN_START:
            case WStype_FRAGMENT:
            case WStype_FRAGMENT_FIN:
                Serial.println("Received fragmented message");
                break;
                
            case WStype_PING:
                Serial.println("Received WebSocket ping");
                break;
                
            case WStype_PONG:
                Serial.println("Received WebSocket pong");
                break;
                
            default:
                Serial.printf("Unknown WebSocket event type: %d\n", type);
                break;
        }
    }
}

void ElevenLabsClient::handleWebSocketMessage(uint8_t* payload, size_t length) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        handleError("Failed to parse JSON message");
        return;
    }
    
    processMessage(doc);
}

void ElevenLabsClient::processMessage(const JsonDocument& doc) {
    String type = doc["type"].as<String>();
    
    if (type == "conversation_initiation_metadata") {
        // Handle conversation initiation
        if (doc["conversation_initiation_metadata_event"].is<JsonObject>()) {
            conversationId = doc["conversation_initiation_metadata_event"]["conversation_id"].as<String>();
            
            Serial.println("Conversation initialized with ID: " + conversationId);
            
            if (conversationInitCallback) {
                conversationInitCallback(conversationId.c_str());
            }
        }
    }
    else if (type == "user_transcript") {
        // Handle user transcript
        if (doc["user_transcription_event"].is<JsonObject>()) {
            String transcript = doc["user_transcription_event"]["user_transcript"].as<String>();
            
            Serial.println("User transcript: " + transcript);
            
            if (transcriptCallback) {
                transcriptCallback(transcript.c_str());
            }
        }
    }
    else if (type == "agent_response") {
        // Handle agent response
        if (doc["agent_response_event"].is<JsonObject>()) {
            String response = doc["agent_response_event"]["agent_response"].as<String>();
            
            Serial.println("Agent response: " + response);
            
            if (agentResponseCallback) {
                agentResponseCallback(response.c_str());
            }
        }
    }
    else if (type == "audio") {
        // Handle audio response - exact Python SDK implementation
        if (doc["audio_event"].is<JsonObject>()) {
            uint32_t event_id = doc["audio_event"]["event_id"].as<uint32_t>();
            
            // Critical: Check for interruption like Python SDK
            if (event_id <= lastInterruptId) {
                Serial.printf("[AUDIO] Skipping audio chunk (Event ID: %u <= Last Interrupt: %u)\n", 
                              event_id, lastInterruptId);
                return;  // Skip this audio chunk
            }
            
            // Check if audio_base_64 field exists
            if (doc["audio_event"]["audio_base_64"].is<String>()) {
                String audioBase64 = doc["audio_event"]["audio_base_64"].as<String>();
                
                Serial.printf("[AUDIO] Processing audio chunk (Event ID: %u, %d chars)\n", 
                              event_id, audioBase64.length());
                
                // Decode base64 to PCM audio (like Python SDK)
                size_t decodedSize = (audioBase64.length() * 3) / 4;  // Estimate decoded size
                uint8_t* pcmData = new uint8_t[decodedSize];
                size_t actualSize = base64Decode(audioBase64.c_str(), pcmData, decodedSize);
                
                if (actualSize > 0) {
                    Serial.printf("[AUDIO] Decoded %d bytes PCM audio\n", actualSize);
                    
                    // Call audio callback with raw PCM data (like Python SDK audio_interface.output)
                    if (audioCallback) {
                        audioCallback(pcmData, actualSize, event_id);
                    }
                } else {
                    Serial.println("[AUDIO] Failed to decode base64 audio");
                }
                
                delete[] pcmData;
            } else {
                Serial.printf("[AUDIO] Received audio event (Event ID: %u) but no audio data found\n", event_id);
            }
        }
    }
    else if (type == "ping") {
        // Handle ping and send pong
        if (doc["ping_event"].is<JsonObject>()) {
            uint32_t event_id = doc["ping_event"]["event_id"].as<uint32_t>();
            uint32_t ping_ms = doc["ping_event"]["ping_ms"].as<uint32_t>();
            
            Serial.printf("Received ping: event_id=%u, ping_ms=%u\n", event_id, ping_ms);
            
            // Send pong response
            sendPong(event_id);
            
            if (pingCallback) {
                pingCallback(event_id, ping_ms);
            }
        }
    }
    else if (type == "client_tool_call") {
        // Handle tool call
        if (doc["client_tool_call"].is<JsonObject>()) {
            String tool_name = doc["client_tool_call"]["tool_name"].as<String>();
            String tool_call_id = doc["client_tool_call"]["tool_call_id"].as<String>();
            
            Serial.println("Tool call: " + tool_name + " (ID: " + tool_call_id + ")");
            
            if (toolCallCallback) {
                JsonDocument params_doc;
                if (doc["client_tool_call"]["parameters"].is<JsonObject>()) {
                    // Copy the parameters object to the new document
                    JsonObjectConst parameters = doc["client_tool_call"]["parameters"].as<JsonObjectConst>();
                    for (JsonPairConst kv : parameters) {
                        params_doc[kv.key().c_str()] = kv.value();
                    }
                }
                toolCallCallback(tool_name.c_str(), tool_call_id.c_str(), params_doc);
            }
        }
    }
    else if (type == "vad_score") {
        // Handle VAD score
        if (doc["vad_score_event"].is<JsonObject>()) {
            float vad_score = doc["vad_score_event"]["vad_score"].as<float>();
            
            if (vadScoreCallback) {
                vadScoreCallback(vad_score);
            }
        }
    }
    else if (type == "internal_tentative_agent_response") {
        // Handle tentative agent response (internal)
        if (doc["tentative_agent_response_internal_event"].is<JsonObject>()) {
            String tentative_response = doc["tentative_agent_response_internal_event"]["tentative_agent_response"].as<String>();
            
            Serial.println("Tentative agent response: " + tentative_response);
        }
    }
    else if (type == "interruption") {
        // Handle interruption - exact Python SDK implementation
        if (doc["interruption_event"].is<JsonObject>()) {
            uint32_t event_id = doc["interruption_event"]["event_id"].as<uint32_t>();
            lastInterruptId = event_id;  // Update last interrupt ID
            
            Serial.printf("[INTERRUPTION] Conversation interrupted (Event ID: %u)\n", event_id);
            
            if (interruptionCallback) {
                interruptionCallback(event_id);
            }
        }
    }
    else if (type == "agent_response_correction") {
        // Handle agent response correction
        if (doc["agent_response_correction_event"].is<JsonObject>()) {
            String corrected_response = doc["agent_response_correction_event"]["agent_response_correction"].as<String>();
            
            Serial.println("Agent response correction: " + corrected_response);
            
            if (agentResponseCallback) {
                agentResponseCallback(corrected_response.c_str());
            }
        }
    }
    else if (type == "interruption") {
        // Handle interruption - exact Python SDK implementation
        if (doc["interruption_event"].is<JsonObject>()) {
            uint32_t event_id = doc["interruption_event"]["event_id"].as<uint32_t>();
            lastInterruptId = event_id;  // Update last interrupt ID
            
            Serial.printf("[INTERRUPTION] Conversation interrupted (Event ID: %u)\n", event_id);
            
            if (interruptionCallback) {
                interruptionCallback(event_id);
            }
        }
    }
    else {
        Serial.println("Unknown message type: " + type);
    }
}

void ElevenLabsClient::handleError(const char* error_message) {
    Serial.println("ElevenLabs Client Error: " + String(error_message));
    
    if (errorCallback) {
        errorCallback(error_message);
    }
}

void ElevenLabsClient::handleDisconnection() {
    connected = false;
    conversationId = "";
    
    Serial.println("WebSocket connection lost. Will attempt to reconnect...");
    
    // Call error callback to notify application
    if (errorCallback) {
        errorCallback("WebSocket connection lost");
    }
}

void ElevenLabsClient::resetReconnectionState() {
    reconnectAttempts = 0;
    lastReconnectAttempt = 0;
    reconnectInterval = 5000; // Reset to initial interval
}

unsigned long ElevenLabsClient::getReconnectDelay() {
    // Exponential backoff: 5s, 10s, 20s, 40s, max 60s
    unsigned long delay = reconnectInterval * (1 << min(reconnectAttempts, 4));
    return min(delay, 60000UL);
}

// Callback registration methods
void ElevenLabsClient::onAudioData(AudioDataCallback callback) {
    audioCallback = callback;
}

void ElevenLabsClient::onTranscript(TranscriptCallback callback) {
    transcriptCallback = callback;
}

void ElevenLabsClient::onAgentResponse(AgentResponseCallback callback) {
    agentResponseCallback = callback;
}

void ElevenLabsClient::onConversationInit(ConversationInitCallback callback) {
    conversationInitCallback = callback;
}

void ElevenLabsClient::onToolCall(ToolCallCallback callback) {
    toolCallCallback = callback;
}

void ElevenLabsClient::onError(ErrorCallback callback) {
    errorCallback = callback;
}

void ElevenLabsClient::onVadScore(VadScoreCallback callback) {
    vadScoreCallback = callback;
}

void ElevenLabsClient::onPing(PingCallback callback) {
    pingCallback = callback;
}

void ElevenLabsClient::onConversationEnd(ConversationEndCallback callback) {
    conversationEndCallback = callback;
}

void ElevenLabsClient::onInterruption(InterruptionCallback callback) {
    interruptionCallback = callback;
}

// Configuration methods
void ElevenLabsClient::setOverrideAudio(bool override) {
    overrideAudio = override;
}

void ElevenLabsClient::enableStreamingAudio(bool enable) {
    streamingAudioEnabled = enable;
    Serial.printf("[WS_CLIENT] Streaming audio %s\n", enable ? "enabled" : "disabled");
}

bool ElevenLabsClient::isStreamingAudioEnabled() {
    return streamingAudioEnabled;
}

// Real-time streaming methods (like Python SDK input_callback)
void ElevenLabsClient::startRealtimeStreaming() {
    if (!connected) {
        Serial.println("[REALTIME] Cannot start streaming: WebSocket not connected");
        return;
    }
    
    streamingAudioEnabled = true;
    Serial.println("[REALTIME] Started real-time audio streaming");
}

void ElevenLabsClient::stopRealtimeStreaming() {
    streamingAudioEnabled = false;
    Serial.println("[REALTIME] Stopped real-time audio streaming");
}

bool ElevenLabsClient::isRealtimeStreaming() {
    return streamingAudioEnabled && connected;
}

void ElevenLabsClient::sendRealtimeAudioChunk(const uint8_t* pcm_data, size_t size) {
    if (!streamingAudioEnabled || !connected) {
        return;
    }
    
    if (!pcm_data || size == 0) {
        return;
    }
    
    // Encode PCM data to base64 (like Python SDK)
    String base64Audio = base64Encode(pcm_data, size);
    if (base64Audio.length() == 0) {
        return;
    }
    
    // Send as real-time chunk (same format as batch)
    JsonDocument doc;
    doc["user_audio_chunk"] = base64Audio;
    
    String message;
    serializeJson(doc, message);
    
    webSocket.sendTXT(message);
}

// Utility Functions
String ElevenLabsClient::base64Encode(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return "";  // Return empty string for invalid input
    }
    
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String encoded = "";
    encoded.reserve((length * 4 / 3) + 4);  // Pre-allocate memory for efficiency
    
    for (size_t i = 0; i < length; i += 3) {
        uint32_t b = (data[i] << 16);
        if (i + 1 < length) b |= (data[i + 1] << 8);
        if (i + 2 < length) b |= data[i + 2];
        
        encoded += base64_chars[(b >> 18) & 0x3F];
        encoded += base64_chars[(b >> 12) & 0x3F];
        encoded += (i + 1 < length) ? base64_chars[(b >> 6) & 0x3F] : '=';
        encoded += (i + 2 < length) ? base64_chars[b & 0x3F] : '=';
    }
    
    return encoded;
}

size_t ElevenLabsClient::base64Decode(const char* base64_string, uint8_t* output_buffer, size_t max_output_size) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t input_len = strlen(base64_string);
    size_t output_len = 0;
    
    if (input_len % 4 != 0) return 0;
    
    for (size_t i = 0; i < input_len && output_len < max_output_size; i += 4) {
        uint32_t b = 0;
        
        for (int j = 0; j < 4; j++) {
            char c = base64_string[i + j];
            if (c == '=') break;
            
            char* pos = strchr((char*)base64_chars, c);
            if (!pos) return 0;
            
            b = (b << 6) | (pos - base64_chars);
        }
        
        if (output_len < max_output_size) output_buffer[output_len++] = (b >> 16) & 0xFF;
        if (output_len < max_output_size && base64_string[i + 2] != '=') output_buffer[output_len++] = (b >> 8) & 0xFF;
        if (output_len < max_output_size && base64_string[i + 3] != '=') output_buffer[output_len++] = b & 0xFF;
    }
    
    return output_len;
}