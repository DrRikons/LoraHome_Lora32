# Project Summary: lora32_boiler_control

*   **Project:** `lora32_boiler_control` - An ESP32-based LoRa sensor network for boiler control.
*   **Architecture:**
    *   **Sensor Node:** Reads temperature/battery, encrypts (AES-128-CTR), and transmits data via LoRa. Enters deep sleep to conserve power. Configuration is updated remotely via LoRa downlink.
    *   **Gateway Node:** Receives sensor data, decrypts it, and publishes to MQTT topics over WiFi (TLS). Manages time synchronization (SNTP/RTC) for the network. Displays sensor and gateway status on a local OLED.
*   **Hardware:** Based on LilyGo T3 V1.6.1 (ESP32 + SX1276).
*   **Communication:**
    *   **LoRa:** Custom binary packed structs: telemetry is 21 bytes (normal) or 31 bytes (development); configuration is 20 bytes with schema version `2`. Gateway controls sensor TX power through configuration; matched firmware images are required.
    *   **MQTT:** Publishes JSON payloads to separate topics for gateway status, sensor operational data, and sensor development telemetry.
        *   `LoRaHome/gateway/status`
        *   `LoRaHome/sensor/<mac>/data`
        *   `LoRaHome/sensor/<mac>/telemetry`
*   **Configuration:**
    *   **Remote (LoRa):** 20-byte schema-v2 binary payload for the one-byte sleep interval, dev mode, TX power, and time synchronization.
    *   **Local:** `DEV_MODE_PIN` (GPIO13) for hardware override into development mode.
    *   **Cloud (MQTT):** Gateway subscribes to `lorahome/sensor/+/config`; signed, non-expired commands override sleep (10–255 seconds), Dev Mode, and signed 8-bit TX-power requests (-128–127 dBm) for known Sensors until Gateway reboot. The Sensor rejects power unsupported by its installed radio. On load, valid SD configuration JSON is completed with missing supported objects and default keys, including `control.commandSecret`.
    *   **Update cadence:** Sensor beacon checks are derived from the active sleep interval, targeting no more than 60 seconds between checks; sleep intervals of 60 seconds or longer check every wake.
    *   **Link metrics:** The Sensor retains the last valid configuration-downlink SNR/RSSI in RTC memory across deep sleep and soft resets for later development telemetry.
    *   **Sleep shutdown:** Before operation-mode deep sleep, the Sensor sleeps the LoRa radio and powers down the INA226. Radio initialization failures retry after a five-minute fail-safe deep sleep.
    *   **Temperature conversion:** The DS18B20 uses 9-bit resolution for 0.5 C steps and a maximum 93.75 ms blocking conversion.
    *   **Low-voltage lockout:** At 3.2 V or below, the Sensor suppresses radio activity and deep-sleeps for one hour. A retained lockout requires recovery to 3.4 V and overrides development mode; a protected battery is still required.
*   **Security:**
    *   **LoRa:** Telemetry is encrypted with AES-128-CTR using the public MAC, per-boot random nonce, and message counter as nonce material. Configuration downlinks are authenticated with a shared `NETWORK_KEY`.
    *   **MQTT:** Communication over TLS.
*   **Development power metric:** The pre-TX INA226 value is the idle baseline. Development mode samples voltage and signed current every 20 ms while LoRa TX and active RX windows run, and reports the signed power average in the existing `battPower` field on the following telemetry cycle; no wire-format change is required.
