# ElevenLabs Conversational AI WebSocket Client

This implementation provides a complete WebSocket client for the ElevenLabs Conversational AI API, specifically designed for ESP32-S3 with 16MB PSRAM. The client supports text message passing, response handling, and audio override functionality.

## Features

### Core WebSocket Functionality
- ✅ WebSocket connection to `wss://api.elevenlabs.io/v1/convai/conversation`
- ✅ Automatic reconnection with configurable intervals
- ✅ Heartbeat/ping-pong mechanism for connection health
- ✅ SSL/TLS support for secure connections
- ✅ Authentication via API key in Authorization header

### Message Types Supported

#### Outgoing Messages (Client → Server)
- ✅ `conversation_initiation_client_data` - Initial connection setup
- ✅ `user_message` - Text messages from user
- ✅ `user_audio_chunk` - Audio data (base64 encoded)
- ✅ `user_activity` - User activity signals
- ✅ `contextual_update` - Context updates
- ✅ `client_tool_result` - Tool execution results
- ✅ `pong` - Response to ping messages

#### Incoming Messages (Server → Client)
- ✅ `conversation_initiation_metadata` - Conversation setup confirmation
- ✅ `user_transcript` - Transcribed user speech
- ✅ `agent_response` - Agent text responses
- ✅ `agent_response_correction` - Corrected agent responses
- ✅ `audio` - Agent audio responses (base64 encoded)
- ✅ `ping` - Server heartbeat (automatically responds with pong)
- ✅ `client_tool_call` - Tool execution requests
- ✅ `vad_score` - Voice Activity Detection scores
- ✅ `internal_tentative_agent_response` - Tentative responses
- ✅ `interruption` - Conversation interruptions

### Configuration Options
- ✅ Audio override (disable default audio handling)
- ✅ Custom agent prompts
- ✅ First message customization
- ✅ Language selection
- ✅ Voice ID configuration
- ✅ Tool handling

### Error Handling
- ✅ Connection error handling
- ✅ JSON parsing error handling
- ✅ Network disconnection recovery
- ✅ WebSocket error event handling
- ✅ User-defined error callbacks

## Usage Example

```cpp
#include "communication/websocket_client.h"

ElevenLabsClient elevenLabs;

void setup() {
    // Configure the client
    elevenLabs.setOverrideAudio(true);
    elevenLabs.setCustomPrompt("You are a helpful assistant.");
    elevenLabs.setFirstMessage("Hello! How can I help?");
    
    // Register callbacks
    elevenLabs.onAgentResponse([](const char* response) {
        Serial.println("Agent: " + String(response));
    });
    
    elevenLabs.onAudioData([](const uint8_t* data, size_t length, uint32_t event_id) {
        // Handle audio data (play through speaker, etc.)
        Serial.printf("Audio received: %u bytes\n", length);
    });
    
    // Start connection
    elevenLabs.begin("your_api_key", "your_agent_id");
}

void loop() {
    elevenLabs.loop();
    
    // Send text message
    if (someCondition) {
        elevenLabs.sendText("Hello, agent!");
    }
}
```

## API Methods

### Connection Management
- `begin(api_key, agent_id)` - Initialize and connect
- `loop()` - Process WebSocket events (call in main loop)
- `disconnect()` - Close connection
- `isConnected()` - Check connection status

### Message Sending
- `sendText(text)` - Send text message
- `sendAudio(data, length)` - Send audio data
- `sendUserActivity()` - Send user activity signal
- `sendContextualUpdate(text)` - Send context update
- `sendToolResult(tool_call_id, result, is_error)` - Send tool result

### Configuration
- `setOverrideAudio(override)` - Enable/disable audio override
- `setCustomPrompt(prompt)` - Set custom agent prompt
- `setFirstMessage(message)` - Set first message
- `setLanguage(language)` - Set conversation language
- `setVoiceId(voice_id)` - Set TTS voice ID

### Callbacks
- `onConversationInit(callback)` - Conversation started
- `onAgentResponse(callback)` - Agent text response
- `onTranscript(callback)` - User speech transcript
- `onAudioData(callback)` - Agent audio response
- `onToolCall(callback)` - Tool execution request
- `onError(callback)` - Error occurred
- `onVadScore(callback)` - Voice activity detection
- `onPing(callback)` - Ping received

## Memory Considerations for ESP32-S3

The implementation is optimized for ESP32-S3 with 16MB PSRAM:
- Dynamic memory allocation for audio buffers
- Efficient JSON parsing with ArduinoJson
- Base64 encoding/decoding for audio data
- String management to minimize fragmentation

## Dependencies

Required libraries (included in platformio.ini):
- `WebSockets` - WebSocket client functionality
- `ArduinoJson` - JSON parsing and serialization
- `base64` - Base64 encoding/decoding for audio

## Notes

1. **Audio Override**: The client automatically sets `override_agent_output_audio: true` to prevent default audio handling, allowing custom audio processing.

2. **Automatic Pong**: The client automatically responds to ping messages to maintain connection health.

3. **Error Recovery**: The WebSocket library handles automatic reconnection on network failures.

4. **Tool Handling**: Implement your tool logic in the `onToolCall` callback and respond with `sendToolResult()`.

5. **Voice Activity Detection**: Use VAD scores to detect when the user is speaking and optimize audio processing.

This implementation provides a complete, production-ready WebSocket client for the ElevenLabs Conversational AI API with comprehensive error handling and all documented message types supported.
