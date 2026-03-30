#include <unity.h>

#include "../support/LoraHomeProtocolModel.h"

using namespace lora_home_test;

void test_end_to_end_sensor_to_gateway_roundtrip() {
    SensorTelemetryInput input{};
    std::uint8_t mac[6] = {0xF0, 0x24, 0xF9, 0xB0, 0x26, 0xAC};
    std::memcpy(input.mac, mac, sizeof(mac));
    input.msgCount = 7;
    input.temperatureC = 23.91f;
    input.batteryVoltageV = 4.144f;
    input.batteryPercent = 98;
    input.configVersion = 3;
    input.devMode = true;
    input.timestamp = static_cast<std::time_t>(CUSTOM_EPOCH + 3600);
    input.batteryCurrentmA = 11.0f;
    input.batteryPowermW = 45.0f;
    input.freeRamKB = 208;
    input.cpuTempC = 58.0f;
    input.txPowerdBm = 17;
    input.lastSNRdB = 4.0f;
    input.lastRSSIdBm = -96.0f;

    TelemetryPayload encoded{};
    std::size_t txSize = 0;
    buildTelemetryPayload(input, encoded, txSize);

    GatewayTelemetryState parsed = parseTelemetryPayload(encoded, txSize);

    TEST_ASSERT_EQUAL_UINT32(TELEMETRY_FULL_SIZE, txSize);
    TEST_ASSERT_EQUAL_UINT16(7, parsed.msgCount);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 23.91f, parsed.temperatureC);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.14f, parsed.batteryVoltageV);
    TEST_ASSERT_EQUAL_UINT8(98, parsed.batteryPercent);
    TEST_ASSERT_EQUAL_UINT8(3, parsed.configVersion);
    TEST_ASSERT_EQUAL_UINT8(1, parsed.opMode);
    TEST_ASSERT_FALSE(parsed.needsTimeSync);
    TEST_ASSERT_TRUE(parsed.hasDevTelemetry);
}

void test_end_to_end_unset_sensor_clock_requests_sync() {
    SensorTelemetryInput input{};
    input.timestamp = 14;
    input.configVersion = 1;

    TelemetryPayload encoded{};
    std::size_t txSize = 0;
    buildTelemetryPayload(input, encoded, txSize);
    GatewayTelemetryState parsed = parseTelemetryPayload(encoded, txSize);

    std::uint32_t gatewayTimeOffset = makeGatewayTimeOffset(true, static_cast<std::time_t>(CUSTOM_EPOCH + 7200));
    TEST_ASSERT_TRUE(parsed.needsTimeSync);
    TEST_ASSERT_TRUE(gatewayShouldSendConfig(parsed.configVersion, parsed.configVersion, parsed.needsTimeSync, gatewayTimeOffset));
}

void test_end_to_end_gateway_without_trusted_time_skips_sync() {
    std::uint32_t gatewayTimeOffset = makeGatewayTimeOffset(false, static_cast<std::time_t>(CUSTOM_EPOCH + 7200));
    TEST_ASSERT_EQUAL_UINT32(0, gatewayTimeOffset);
    TEST_ASSERT_FALSE(gatewayShouldSendConfig(1, 1, true, gatewayTimeOffset));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_end_to_end_sensor_to_gateway_roundtrip);
    RUN_TEST(test_end_to_end_unset_sensor_clock_requests_sync);
    RUN_TEST(test_end_to_end_gateway_without_trusted_time_skips_sync);
    return UNITY_END();
}
