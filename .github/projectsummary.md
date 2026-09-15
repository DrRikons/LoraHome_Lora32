# Project Summary: lora32_boiler_control

*   **Project:** `lora32_boiler_control` - An ESP32-based LoRa sensor network for boiler control.
*   **Architecture:**
    *   **Sensor Node:** Reads temperature/battery, encrypts (AES-128-CTR), and transmits data via LoRa. Enters deep sleep to conserve power. Configuration is updated remotely via LoRa downlink.
    *   **Gateway Node:** Receives sensor data, decrypts it, and publishes to MQTT topics over WiFi (TLS). Manages time synchronization (SNTP/RTC) for the network. Displays sensor and gateway status on a local OLED.
*   **Hardware:** Based on LilyGo T3 V1.6.1 (ESP32 + SX1276).
*   **Communication:**
    *   **LoRa:** Custom binary packed structs: telemetry is 20 bytes (normal) or 31 bytes (development); configuration is 19 bytes with schema version `1`.
    *   **MQTT:** Publishes JSON payloads to separate topics for gateway status, sensor operational data, and sensor development telemetry.
        *   `LoRaHome/gateway/status`
        *   `LoRaHome/sensor/<mac>/data`
        *   `LoRaHome/sensor/<mac>/telemetry`
*   **Configuration:**
    *   **Remote (LoRa):** 19-byte binary payload for sleep interval, dev mode, and time synchronization.
    *   **Local:** `DEV_MODE_PIN` (GPIO13) for hardware override into development mode.
*   **Security:**
    *   **LoRa:** Telemetry is encrypted with AES-128-CTR using the public MAC, per-boot random nonce, and message counter as nonce material. Configuration downlinks are authenticated with a shared `NETWORK_KEY`.
    *   **MQTT:** Communication over TLS.
