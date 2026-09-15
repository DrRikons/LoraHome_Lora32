#pragma once

#ifndef LORAHOMECOMMON_H
#define LORAHOMECOMMON_H

#include <Arduino.h> // For uint8_t, uint16_t, etc.
#include <ArduinoJson.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

// Base time offset to reduce LoRa payload sizes (Jan 1, 2024 00:00:00 UTC)
#define CUSTOM_EPOCH 1704067200UL

constexpr uint8_t CONFIG_UPDATE_PENDING_FLAG = 0xA5;

// Shared by Sensor and GateWay: true if currentTime has diverged from the boot-epoch
// estimate (CUSTOM_EPOCH + uptimeSeconds), meaning a real clock sync has occurred.
// Callers supply their own uptimeSeconds source (deep-sleeping devices vs long-running ones).

inline bool isClockValidSince(time_t currentTime, unsigned long uptimeSeconds) {
    time_t expectedBootTime = (time_t)CUSTOM_EPOCH + (time_t)uptimeSeconds;
    if (labs((long)(currentTime - expectedBootTime)) <= 10 || currentTime < (time_t)CUSTOM_EPOCH) {
        return false;
    }
    return true;
}

inline bool hasClockDrift(time_t currentTime, time_t referenceTime, double thresholdSeconds) {
    return fabs(difftime(currentTime, referenceTime)) > thresholdSeconds;
}

#define TELEMETRY_CORE_FIELDS(FIELD, ARRAY, STRING) \
    ARRAY(uint8_t,  mac, 6, "mac") \
    FIELD(uint32_t, bootNonce, "bootNonce") \
    FIELD(uint16_t, msgCount, "msgCount") \
    FIELD(int16_t,  temperature, "temperature") \
    FIELD(uint16_t, battVoltage, "battVoltage") \
    FIELD(uint8_t,  battPercent, "battPercent") \
    FIELD(uint8_t,  isDevMode, "isDevMode") \
    FIELD(uint8_t,  needsTimeSync, "needsTimeSync") \
    FIELD(uint8_t,  sleepInterval, "sleepInterval")

#define TELEMETRY_DEV_FIELDS(FIELD, ARRAY, STRING) \
    FIELD(int16_t,  battCurrent, "battCurrent") \
    FIELD(int16_t,  battPower, "battPower") \
    FIELD(uint16_t, freeRam, "freeRam") \
    FIELD(int8_t,   cpuTemp, "cpuTemp") \
    FIELD(int8_t,   txPower, "txPower") \
    FIELD(int8_t,   lastSNR, "lastSNR") \
    FIELD(int16_t,  lastRSSI, "lastRSSI")

#define CONFIG_PAYLOAD(FIELD, ARRAY, STRING) \
   STRING(header, 2, "header") \
   ARRAY(uint8_t,  targetMac, 6, "targetMac") \
   FIELD(uint32_t, networkKey, "networkKey") \
   FIELD(uint8_t,  sleepInterval, "sleepInterval") \
   FIELD(uint8_t,  configVersion, "configVersion") \
   FIELD(uint8_t,  isDevMode, "isDevMode") \
   FIELD(uint32_t, timeOffset, "timeOffset")

#define GATEWAY_STATUS(FIELD, ARRAY, STRING) \
   FIELD(int16_t,  gatewayRssi, "gatewayRssi") \
   FIELD(float,    gatewaySnr, "gatewaySnr") \
   STRING(wifiStatus, 20, "wifiStatus") \
   STRING(ip, 16, "ip") \
   FIELD(uint32_t, freeRam, "freeRam")

// Define how to declare a variable inside the struct
#define DECLARE_FIELD(type, name, json_key) type name;
#define DECLARE_ARRAY(type, name, size, json_key) type name[size];
#define DECLARE_STRING(name, size, json_key) char name[size];

struct __attribute__((packed)) TelemetryPayload {
    // --- Core variables ---
    TELEMETRY_CORE_FIELDS(DECLARE_FIELD, DECLARE_ARRAY, DECLARE_STRING)
    
    // --- Dev mode variables ---
    TELEMETRY_DEV_FIELDS(DECLARE_FIELD, DECLARE_ARRAY, DECLARE_STRING)

};

struct __attribute__((packed)) ConfigPayload {

    // --- Config Payload ---
    CONFIG_PAYLOAD(DECLARE_FIELD, DECLARE_ARRAY, DECLARE_STRING)

};

struct __attribute__((packed)) GatewayStatus {

    // --- Config Payload ---
    GATEWAY_STATUS(DECLARE_FIELD, DECLARE_ARRAY, DECLARE_STRING)

};

static_assert(sizeof(TelemetryPayload) == 31, "TelemetryPayload wire format must be 31 bytes");
static_assert(sizeof(ConfigPayload) == 19, "ConfigPayload wire format must be 19 bytes");

// ============================================================================
// JSON Serialization Macros - MUST stay defined until after all uses!
// ============================================================================

// #define TO_JSON_FIELD(type, name, json_key) \
//     doc[json_key] = payload.name;

// #define TO_JSON_ARRAY(type, name, size, json_key) \
//     { \
//         JsonArray arr = doc[json_key].to<JsonArray>(); \
//         for(int i = 0; i < size; i++) { arr.add(payload.name[i]); } \
//     }

// Safely extract a scalar: Only update if the key exists
#define FROM_JSON_FIELD(type, name, json_key) \
    if (doc.containsKey(json_key)) { \
        payload.name = doc[json_key].as<type>(); \
    }

// Safely extract an array: Check type and enforce strict bounds limits
#define FROM_JSON_ARRAY(type, name, max_len, json_key) \
    if (doc.containsKey(json_key) && doc[json_key].is<JsonArrayConst>()) { \
        JsonArrayConst arr = doc[json_key].as<JsonArrayConst>(); \
        size_t itemsToCopy = (arr.size() < max_len) ? arr.size() : max_len; \
        for(size_t i = 0; i < itemsToCopy; i++) { \
            payload.name[i] = arr[i].as<type>(); \
        } \
    }

#define FROM_JSON_STRING(name, max_len, json_key) \
    if (doc.containsKey(json_key) && doc[json_key].is<const char*>()) { \
        strncpy(payload.name, doc[json_key].as<const char*>(), max_len); \
    }

// ============================================================================
// Generic Deserialization function - uses FROM_JSON_FIELD and FROM_JSON_ARRAY
// ============================================================================
 inline bool deserializeJson(void* rawPayload, const JsonDocument& doc, const char* mode) {
    if (!rawPayload || !mode) return false;
    
    switch(mode[0]) {
        case 'c': { // "core"
            TelemetryPayload& payload = *static_cast<TelemetryPayload*>(rawPayload);
            TELEMETRY_CORE_FIELDS(FROM_JSON_FIELD, FROM_JSON_ARRAY, FROM_JSON_STRING)
            break;
        }
        case 'd': { // "dev"
            TelemetryPayload& payload = *static_cast<TelemetryPayload*>(rawPayload);
            TELEMETRY_DEV_FIELDS(FROM_JSON_FIELD, FROM_JSON_ARRAY, FROM_JSON_STRING)
            break;
        }
        case 'g': { // "gatewayStatus"
            GatewayStatus& payload = *static_cast<GatewayStatus*>(rawPayload);
            GATEWAY_STATUS(FROM_JSON_FIELD, FROM_JSON_ARRAY, FROM_JSON_STRING)
            break;
        }
        case 'p': { // "payloadConfig"
            ConfigPayload& payload = *static_cast<ConfigPayload*>(rawPayload);
            CONFIG_PAYLOAD(FROM_JSON_FIELD, FROM_JSON_ARRAY, FROM_JSON_STRING)
            break;
        }
        default:
            return false;
    }
    return true;
}

// ============================================================================
// CLEANUP - Remove all macros at END of file to prevent namespace pollution
// ============================================================================
#undef DECLARE_FIELD  
#undef DECLARE_ARRAY  
#undef DECLARE_STRING
// #undef TO_JSON_FIELD
// #undef TO_JSON_ARRAY
#undef FROM_JSON_FIELD
#undef FROM_JSON_ARRAY   
#undef FROM_JSON_STRING

#endif // LORAHOMECOMMON_H
