#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

/**
 * @class WiFiManager
 * @brief Manages the Wi-Fi connection for the device.
 *
 * This class encapsulates all functionality related to connecting to a Wi-Fi
 * network, checking the connection status, and handling timeouts.
 */
class WiFiManager {
    public:
        /**
        * @brief Attempts to connect to a Wi-Fi network with a timeout.
        * @param ssid The SSID of the Wi-Fi network.
        * @param password The password for the Wi-Fi network.
        * @param timeout_ms The maximum time to wait for a connection, in milliseconds.
        * @return true if the connection was successful, false otherwise.
        */
        bool connect(const char* ssid, const char* password, unsigned long timeout_ms = 10000);

        /**
        * @brief Checks if the device is currently connected to Wi-Fi.
        * @return true if connected, false otherwise.
        */
        bool isConnected();
};

#endif