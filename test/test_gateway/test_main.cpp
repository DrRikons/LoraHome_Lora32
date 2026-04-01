#include <unity.h>

#include "../support/LoraHomeProtocolModel.h"

using namespace lora_home_test;

void test_gateway_parses_core_payload_fields() {
    TelemetryPayload payload{};
    std::uint8_t mac[6] = {0xF0, 0x24, 0xF9, 0xB0, 0x26, 0xAC};
    std::memcpy(payload.mac, mac, sizeof(mac));
    payload.msgCount = 2;
    payload.temperature = 2390;
    payload.battVoltage = 4140;
    payload.battPercent = 99;
    payload.isDevMode = 1;
    payload.needsTimeSync = 1;
    payload.sleepInterval = 300;

    GatewayTelemetryState parsed = parseTelemetryPayload(payload, TELEMETRY_CORE_SIZE);

    TEST_ASSERT_EQUAL_STRING("F0:24:F9:B0:26:AC", parsed.macString);
    TEST_ASSERT_EQUAL_UINT16(2, parsed.msgCount);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 23.9f, parsed.temperatureC);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.14f, parsed.batteryVoltageV);
    TEST_ASSERT_EQUAL_UINT8(99, parsed.batteryPercent);
    TEST_ASSERT_TRUE(parsed.needsTimeSync);
    TEST_ASSERT_EQUAL_UINT32(300, parsed.sleepIntervalSeconds);
    TEST_ASSERT_FALSE(parsed.hasDevTelemetry);
}

void test_gateway_parses_dev_payload_fields() {
    TelemetryPayload payload{};
    payload.needsTimeSync = 0;
    payload.battCurrent = 12;
    payload.battPower = 34;
    payload.freeRam = 211;
    payload.cpuTemp = 59;
    payload.txPower = 17;
    payload.lastSNR = -2;
    payload.lastRSSI = -103;

    GatewayTelemetryState parsed = parseTelemetryPayload(payload, TELEMETRY_FULL_SIZE);

    TEST_ASSERT_TRUE(parsed.hasDevTelemetry);
    TEST_ASSERT_EQUAL_INT16(12, parsed.batteryCurrentmA);
    TEST_ASSERT_EQUAL_INT16(34, parsed.batteryPowermW);
    TEST_ASSERT_EQUAL_UINT16(211, parsed.freeRamKB);
    TEST_ASSERT_EQUAL_INT8(59, parsed.cpuTempC);
    TEST_ASSERT_EQUAL_INT8(17, parsed.txPowerdBm);
    TEST_ASSERT_EQUAL_INT8(-2, parsed.lastSNRdB);
    TEST_ASSERT_EQUAL_INT16(-103, parsed.lastRSSIdBm);
}

void test_gateway_config_decision_uses_state_or_sync_flag() {
    TEST_ASSERT_TRUE(gatewayShouldSendConfig(60, false, 120, false, false, 123));
    TEST_ASSERT_TRUE(gatewayShouldSendConfig(60, false, 60, true, false, 123));
    TEST_ASSERT_TRUE(gatewayShouldSendConfig(60, false, 60, false, true, 123));
    TEST_ASSERT_FALSE(gatewayShouldSendConfig(60, false, 60, false, true, 0));
    TEST_ASSERT_FALSE(gatewayShouldSendConfig(60, false, 60, false, false, 123));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_gateway_parses_core_payload_fields);
    RUN_TEST(test_gateway_parses_dev_payload_fields);
    RUN_TEST(test_gateway_config_decision_uses_state_or_sync_flag);
    return UNITY_END();
}
