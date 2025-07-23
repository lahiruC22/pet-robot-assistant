#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <time.h>

WiFiMulti wifiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

// Replace these with your credentials
const char* ssid = "KS-device";
const char* password = "qwerty999";

String api = "sk_74e2633e42a5669fadb531d0766dce352c5f267ef918fb4f";
String agent_id = "agent_01k0nta4ekfj4868162kx0g5x5";

// Setup NTP for TLS certificate validation
void setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  USE_SERIAL.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    USE_SERIAL.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  USE_SERIAL.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  USE_SERIAL.print(F("Current time: "));
  USE_SERIAL.print(asctime(&timeinfo));
}

String userInput = "Hi how are you?";
String jsonPayload = "{\"type\":\"user_message\",\"text\":\"" + userInput + "\"}";

// WebSocket Event Handler
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;

    case WStype_CONNECTED:
      USE_SERIAL.printf("[WSc] Connected to ElevenLabs WebSocket\n");
      webSocket.sendTXT(jsonPayload);
      USE_SERIAL.println("[SEND] " + userInput);
      break;

    case WStype_TEXT:
    {
      USE_SERIAL.printf("[WSc] Raw Response: %s\n", payload);

      // Parse the JSON reply
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        USE_SERIAL.print(F("[WSc] Failed to parse JSON: "));
        USE_SERIAL.println(error.f_str());
        break;
      }

      const char* type = doc["type"];
      const char* text = doc["text"];

      if (type && strcmp(type, "reply") == 0 && text) {
        USE_SERIAL.println("\n========== AI REPLY ==========");
        USE_SERIAL.println(text);
        USE_SERIAL.println("================================\n");
      } else {
        USE_SERIAL.println("[WSc] Non-reply message received:");
        serializeJsonPretty(doc, Serial);
        Serial.println();
      }

      break;
    }

    case WStype_BIN:
      USE_SERIAL.printf("[WSc] Received binary data (%d bytes)\n", length);
      break;

    case WStype_ERROR:
      USE_SERIAL.println("[WSc] Error");
      break;

    default:
      break;
  }
}

void setup() {
  USE_SERIAL.begin(115200);
  USE_SERIAL.println();
  USE_SERIAL.print("esp_arduino version: ");
  USE_SERIAL.println(ESP_ARDUINO_VERSION);
  USE_SERIAL.setDebugOutput(false);

  USE_SERIAL.println("\n[SETUP] Connecting to WiFi...");
  wifiMulti.addAP(ssid, password);
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  USE_SERIAL.println("[SETUP] WiFi connected.");
  delay(200);
  setClock();
  USE_SERIAL.println("=============== Clock Setup Completed ===============");

  // Prepare Authorization Header
  String headers = "Authorization: Bearer " + api + "\r\n";
  webSocket.setExtraHeaders(headers.c_str());
  USE_SERIAL.println("[SETUP] API key sent");

  // Connect to ElevenLabs WebSocket
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 4)
  webSocket.beginSslWithBundle(
    "api.elevenlabs.io",
    443,
    ("/v1/convai/conversation?agent_id=" + agent_id).c_str(),
    NULL, 0, ""
  );
#else
  webSocket.beginSslWithBundle(
    "api.elevenlabs.io",
    443,
    ("/v1/convai/conversation?agent_id=" + agent_id).c_str(),
    NULL, ""
  );
#endif

  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  USE_SERIAL.println("[SETUP] Ready. Type your message below:");
}

void loop() {
  webSocket.loop();
}
