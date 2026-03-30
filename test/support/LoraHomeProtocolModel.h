#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace lora_home_test {

constexpr std::uint32_t CUSTOM_EPOCH = 1704067200UL;
constexpr std::size_t TELEMETRY_CORE_SIZE = 16;
constexpr std::size_t TELEMETRY_FULL_SIZE = 27;
constexpr std::size_t CONFIG_PAYLOAD_SIZE = 22;

#pragma pack(push, 1)
struct TelemetryPayload {
    std::uint8_t mac[6];
    std::uint16_t msgCount;
    std::int16_t temperature;
    std::uint16_t battVoltage;
    std::uint8_t battPercent;
    std::uint8_t configVer;
    std::uint8_t opMode;
    std::uint8_t needsTimeSync;
    std::int16_t battCurrent;
    std::int16_t battPower;
    std::uint16_t freeRam;
    std::int8_t cpuTemp;
    std::int8_t txPower;
    std::int8_t lastSNR;
    std::int16_t lastRSSI;
};

struct ConfigPayload {
    char header[2];
    std::uint8_t targetMac[6];
    std::uint32_t networkKey;
    std::uint32_t sleepInterval;
    std::uint8_t configVersion;
    std::uint8_t isDevMode;
    std::uint32_t timeOffset;
};
#pragma pack(pop)

static_assert(sizeof(TelemetryPayload) == TELEMETRY_FULL_SIZE, "Telemetry payload layout changed");
static_assert(sizeof(ConfigPayload) == CONFIG_PAYLOAD_SIZE, "Config payload layout changed");

inline bool isClockValid(std::time_t currentTime) {
    return currentTime > static_cast<std::time_t>(CUSTOM_EPOCH);
}

inline bool hasClockDrift(std::time_t currentTime, std::time_t referenceTime, double thresholdSeconds) {
    return std::fabs(std::difftime(currentTime, referenceTime)) > thresholdSeconds;
}

struct SensorTelemetryInput {
    std::uint8_t mac[6];
    std::uint16_t msgCount;
    float temperatureC;
    float batteryVoltageV;
    std::uint8_t batteryPercent;
    std::uint8_t configVersion;
    bool devMode;
    std::time_t timestamp;
    float batteryCurrentmA;
    float batteryPowermW;
    std::uint16_t freeRamKB;
    float cpuTempC;
    std::int8_t txPowerdBm;
    float lastSNRdB;
    float lastRSSIdBm;
};

struct GatewayTelemetryState {
    char macString[18];
    std::uint16_t msgCount;
    float temperatureC;
    float batteryVoltageV;
    std::uint8_t batteryPercent;
    std::uint8_t configVersion;
    std::uint8_t opMode;
    bool needsTimeSync;
    bool hasDevTelemetry;
    std::int16_t batteryCurrentmA;
    std::int16_t batteryPowermW;
    std::uint16_t freeRamKB;
    std::int8_t cpuTempC;
    std::int8_t txPowerdBm;
    std::int8_t lastSNRdB;
    std::int16_t lastRSSIdBm;
};

inline void buildTelemetryPayload(const SensorTelemetryInput& input, TelemetryPayload& output, std::size_t& txSize) {
    std::memset(&output, 0, sizeof(output));
    std::memcpy(output.mac, input.mac, sizeof(output.mac));
    output.msgCount = input.msgCount;
    output.temperature = static_cast<std::int16_t>(input.temperatureC * 100.0f);
    output.battVoltage = static_cast<std::uint16_t>(input.batteryVoltageV * 1000.0f);
    output.battPercent = input.batteryPercent;
    output.configVer = input.configVersion;
    output.opMode = input.devMode ? 1 : 0;
    output.needsTimeSync = isClockValid(input.timestamp) ? 0 : 1;

    txSize = TELEMETRY_CORE_SIZE;
    if (input.devMode) {
        output.battCurrent = static_cast<std::int16_t>(input.batteryCurrentmA);
        output.battPower = static_cast<std::int16_t>(input.batteryPowermW);
        output.freeRam = input.freeRamKB;
        output.cpuTemp = static_cast<std::int8_t>(input.cpuTempC);
        output.txPower = input.txPowerdBm;
        output.lastSNR = static_cast<std::int8_t>(input.lastSNRdB);
        output.lastRSSI = static_cast<std::int16_t>(input.lastRSSIdBm);
        txSize = sizeof(TelemetryPayload);
    }
}

inline GatewayTelemetryState parseTelemetryPayload(const TelemetryPayload& payload, std::size_t packetSize) {
    GatewayTelemetryState state{};
    std::snprintf(
        state.macString,
        sizeof(state.macString),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        payload.mac[0], payload.mac[1], payload.mac[2],
        payload.mac[3], payload.mac[4], payload.mac[5]
    );
    state.msgCount = payload.msgCount;
    state.temperatureC = payload.temperature / 100.0f;
    state.batteryVoltageV = payload.battVoltage / 1000.0f;
    state.batteryPercent = payload.battPercent;
    state.configVersion = payload.configVer;
    state.opMode = payload.opMode;
    state.needsTimeSync = payload.needsTimeSync != 0;
    state.hasDevTelemetry = packetSize == sizeof(TelemetryPayload);
    if (state.hasDevTelemetry) {
        state.batteryCurrentmA = payload.battCurrent;
        state.batteryPowermW = payload.battPower;
        state.freeRamKB = payload.freeRam;
        state.cpuTempC = payload.cpuTemp;
        state.txPowerdBm = payload.txPower;
        state.lastSNRdB = payload.lastSNR;
        state.lastRSSIdBm = payload.lastRSSI;
    }
    return state;
}

inline std::uint32_t makeGatewayTimeOffset(bool hasTrustedRtc, std::time_t localTime) {
    if (!hasTrustedRtc || !isClockValid(localTime)) {
        return 0;
    }
    return static_cast<std::uint32_t>(localTime - static_cast<std::time_t>(CUSTOM_EPOCH));
}

inline bool gatewayShouldSendConfig(
    std::uint8_t sensorConfigVersion,
    std::uint8_t targetConfigVersion,
    bool sensorNeedsTimeSync,
    std::uint32_t gatewayTimeOffset
) {
    return (sensorConfigVersion != targetConfigVersion) || (gatewayTimeOffset > 0 && sensorNeedsTimeSync);
}

}  // namespace lora_home_test
