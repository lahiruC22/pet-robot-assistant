#include <WiFi.h>
#include <WebSocketsClient.h>
#include <base64.h> // For encoding the audio data

/************************* CONFIGURATION *********************************/

// --- Wi-Fi Credentials ---
#define WLAN_SSID "KS-device"
#define WLAN_PASS "qwerty999"

// --- WebSocket STT API Example Values (REPLACE WITH YOUR ACTUAL API) ---
const char* WS_HOST = "api.elevenlabs.io"; // The server HOSTNAME (no "ws://")
const int   WS_PORT = 80;                  // The server port
const char* WS_PATH = "/v1/convai/conversation?agent_id=agent_01jzbynanrem99xpncv8ef0m8y"; // The endpoint path
const char* API_KEY = "YOUR_SUPER_SECRET_API_KEY"; // An API key, if needed for headers

const char* PREDEFINED_BASE64_STRING = "SGVsbG8gV2ViU29ja2V0IQ=="; // "Hello WebSocket!"

/************************* GLOBAL OBJECTS *********************************/

WebSocketsClient webSocket;

String responseBase64 = "";

/*************************** FUNCTION PROTOTYPES **************************/
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

/*************************** SETUP ****************************************/

void setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure WebSocket client
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  
  // // Set an authorization header (many APIs use this)
  // // Format: "HeaderName: HeaderValue"
  // webSocket.setExtraHeaders(("Authorization: Bearer " + String(API_KEY)).c_str());

  // Register the event handler function
  webSocket.onEvent(webSocketEvent);

  // Set a reconnect interval in case of disconnection
  webSocket.setReconnectInterval(5000); // try to reconnect every 5s
}

/*************************** LOOP *****************************************/

void loop() {
  // The magic happens here. This function handles all WebSocket communication,
  // including pings, disconnects, and calling our event handler.
  webSocket.loop();
}

//==============================================================
// --- WEBSOCKET EVENT HANDLER (MODIFIED) ---
//==============================================================

/**
 * @brief Handles incoming WebSocket events.
 */
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WebSocket] Disconnected!");
      break;

    case WStype_CONNECTED:
      Serial.printf("[WebSocket] Connected to server: %s\n", payload);
      
      // --- SEND THE PREDEFINED STRING UPON CONNECTION ---
      Serial.printf("--> Sending Base64 string: %s\n", PREDEFINED_BASE64_STRING);
      webSocket.sendTXT(PREDEFINED_BASE64_STRING);
      break;

    case WStype_TEXT:
      // --- THIS IS THE MODIFIED SECTION ---
      Serial.printf("<-- Received text: %s\n", payload);

      // Store the received payload into our global variable.
      // We cast the uint8_t* payload to a char* so the String object can use it.
      responseBase64 = (char*)payload;

      // Print the content of the variable to confirm it was stored correctly.
      Serial.println("  > Returned response: ");
      Serial.println(responseBase64);
      Serial.println("------------------------------------");
      break;

    case WStype_BIN:
      Serial.printf("<-- Received binary data of length: %u\n", length);
      break;
      
    default:
      break;
  }
}