#include "websocket_client.h"
#include <ArduinoJson.h>

// Allows the static router function to call the class instance method.
static ElevenLabsClient* instance = nullptr;

void ElevenLabsClient::webSocketEventRouter(WStype_t type, uint8_t* payload, size_t length) {
    if (instance) {
        instance->webSocketEvent(type, payload, length);
    }
}

ElevenLabsClient::ElevenLabsClient() {
    instance = this;
}

void ElevenLabsClient::begin(const char* agentId, const char* api_key) {
    this->agentId = agentId;
    this->apikey = api_key;

    String url = "/v1/convai/conversation?agent_id=" + this->agentId;

    // Connect to ElevenLabs WebSocket server with SSL
    WebSocket.beginSSL("api.elevenlabs.io", 443, url.c_str());

    // Register the event handler
    WebSocket.onEvent(webSocketEventRouter);
    WebSocket.setReconnectInterval(5000);
}

void ElevenLabsClient::loop() {
    WebSocket.loop();
}

bool ElevenLabsClient::isConnected() {
    return this -> connected;
}

void ElevenLabsClient::webSocketEvent(WStype_t type, uint8_t* payload, size_t length){
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSc] Disconnected!");
            this->connected = false;
            if (connectionCallback) connectionCallback(false);
        break;
        
        case WStype_CONNECTED:
            Serial.println("[WSc] Connected to URL. Sending auth...");
            {
                // STEP 1: Send the initial message with API key for
                // Authorization

                JsonDocument doc;
                doc["type"] = "setup_config";
                doc["xi_api_key"] = this->apikey;

                String jsonPayload;
                serializeJson(doc, jsonPayload);
                WebSocket.sendTXT(jsonPayload);
            }
        break;

        case WStype_TEXT: {
            Serial.printf("[WSc] Raw Response: %s\n", payload);

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (error) {
                Serial.printf("[WSc] Failed to parse JSON: ");
                Serial.println(error.c_str());
                break;
            }

            const char* msgType = doc["type"];

            if (strcmp(msgType, "connection_opened") == 0){
                this->connected = true;
                Serial.println("[WSc] Authorization successful. Connection opened.");
                if (connectionCallback) connectionCallback(true);
            } else if (strcmp(msgType, "reply") == 0 && messageCallback ){
                messageCallback(msgType, doc["text"]);
            } else if (strcmp(msgType, "error") == 0 && messageCallback) {
                messageCallback(msgType, doc["message"]);
            }
        }
        break;

        case WStype_BIN:
            Serial.printf("[WSc] Received binary data (%d bytes)\n", length);
            if (audioDataCallback) {
                audioDataCallback(payload, length);
            }
            break;

        case WStype_ERROR:
        case WStype_PONG:
        case WStype_PING:
            break;
    }
}

void ElevenLabsClient::sendText(const char* text){
    if(!isConnected()){
        Serial.println("[WSc] Cannot send text, not connected.");
        return;
    }

    JsonDocument doc;
    doc["type"] = "user_message";
    doc["text"] = text;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    WebSocket.sendTXT(jsonPayload);
    Serial.print("[SEND]");
    Serial.println(text);
}

void ElevenLabsClient::onMessage(MessageCallback callback) {
    this->messageCallback = callback;
}

void ElevenLabsClient::onAudioData(AudioDataCallback callback) {
    this->audioDataCallback = callback;
}

void ElevenLabsClient::onConnection(ConnectionCallback callback) {
    this->connectionCallback = callback;
}