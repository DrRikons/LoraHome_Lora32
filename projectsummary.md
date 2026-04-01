# Project Summary

## Recent Changes
- Fixed ESP32 compilation errors in `modes/GateWay/main.cpp`.
- Replaced deprecated `ICACHE_RAM_ATTR` with `IRAM_ATTR` for ESP32.
- Handled NTP synchronization validation without relying on the non-standard `timeSync()` function.
- Added cross-version compatibility support for `ArduinoJson` (v6 and v7).
- Replaced `Serial.print` with native ESP-IDF `esp_log` macros, redirecting to an asynchronous FreeRTOS queue for non-blocking SD card logging.
- Removed redundant function name injection (`[%s]`, `__func__`) from `ESP_LOG` statements.
- Formatted human-readable date strings into the MQTT telemetry payload if the hardware clock is synced.
- Prepended local datetime stamps to all asynchronous `esp_log` outputs (UART and SD card) when the clock is valid.