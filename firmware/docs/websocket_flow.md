# ElevenLabs WebSocket Conversation Flow

## Connection and First Message Flow

### 1. WebSocket Connection
```
ESP32 -> ElevenLabs: WebSocket connection to wss://api.elevenlabs.io/v1/convai/conversation?agent_id=YOUR_AGENT_ID
```

### 2. Initial Handshake
```
ESP32 -> ElevenLabs: {"type":"conversation_initiation_client_data","conversation_config_override":{"override_agent_output_audio":true}}
ElevenLabs -> ESP32: {"type":"conversation_initiation_metadata","conversation_initiation_metadata_event":{"conversation_id":"conv_123"}}
```

### 3. Agent First Message (Automatic)
```
ElevenLabs -> ESP32: {"type":"agent_response","agent_response_event":{"agent_response":"Hello! I'm your assistant. How can I help you today?"}}
```
**This is the first message you configured in the web interface and will be automatically sent after connection.**

### 4. User Text Message
```
User types in Serial Monitor: "What's the weather like?"
ESP32 -> ElevenLabs: {"type":"user_message","text":"What's the weather like?"}
```

### 5. Agent Response
```
ElevenLabs -> ESP32: {"type":"agent_response","agent_response_event":{"agent_response":"I'd be happy to help with weather information..."}}
```

## Key Points

- **First Message**: Configured in ElevenLabs web interface, automatically sent after connection
- **No API Key**: Public agents don't require authentication
- **Text Focus**: Audio processing is disabled, focusing on text conversations
- **Interactive**: Type messages in Serial Monitor to chat with the agent
- **Callbacks**: All events trigger appropriate callback functions in main.cpp

## Usage

1. Flash the firmware to ESP32-S3
2. Open Serial Monitor (115200 baud)
3. Wait for "Wi-Fi connected successfully!" and "Setup complete!"
4. The agent's first message will appear automatically
5. Type your messages in the Serial Monitor to continue the conversation
