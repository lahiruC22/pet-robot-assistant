#include <Arduino.h>
#include <unity.h>
#include "communication/wifi_manager.h"

WiFiManager wifiManager;

void setUp(void) {
    WiFi.disconnect();
    delay(100);
}

void tearDown(void) {
    // Clean up after each test
}

void test_wifi_connect_success() {
    bool connected = wifiManager.connect("Wokwi-GUEST", "", 15000);
    TEST_ASSERT_TRUE_MESSAGE(connected, "Failed to connect with valid credentials");
    TEST_ASSERT_TRUE_MESSAGE(wifiManager.isConnected(),"isConnected returned false after successfull connection");
}

void test_wifi_connect_failure_bad_credentials(void) {
    bool connected = wifiManager.connect("InvalidSSID", "bad-password", 5000);
    TEST_ASSERT_FALSE_MESSAGE(connected, "Connected with invalid credentials");
    TEST_ASSERT_FALSE_MESSAGE(wifiManager.isConnected(), "isConnected returned true after failed connection");
}

void setup() {
    delay(2000);

    UNITY_BEGIN();
    RUN_TEST(test_wifi_connect_success);
    RUN_TEST(test_wifi_connect_failure_bad_credentials);

    UNITY_END();
}

void loop() {
    // Nothing to do here
}