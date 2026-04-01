#include <unity.h>

#include "../support/LoraHomeProtocolModel.h"

using namespace lora_home_test;

void test_sensor_marks_unset_clock_for_sync() {
    SensorTelemetryInput input{};
    input.timestamp = 14;
    input.sleepIntervalSeconds = 60;

    TelemetryPayload payload{};
    std::size_t txSize = 0;
    buildTelemetryPayload(input, payload, txSize);

    TEST_ASSERT_EQUAL_UINT8(1, payload.needsTimeSync);
    TEST_ASSERT_EQUAL_UINT32(TELEMETRY_CORE_SIZE, txSize);
}

void test_sensor_marks_valid_clock_as_synced() {
    SensorTelemetryInput input{};
    input.timestamp = static_cast<std::time_t>(CUSTOM_EPOCH + 60);
    input.sleepIntervalSeconds = 60;

    TelemetryPayload payload{};
    std::size_t txSize = 0;
    buildTelemetryPayload(input, payload, txSize);

    TEST_ASSERT_EQUAL_UINT8(0, payload.needsTimeSync);
}

void test_sensor_uses_compact_and_full_sizes() {
    SensorTelemetryInput input{};
    input.sleepIntervalSeconds = 60;
    TelemetryPayload payload{};
    std::size_t txSize = 0;

    input.devMode = false;
    buildTelemetryPayload(input, payload, txSize);
    TEST_ASSERT_EQUAL_UINT32(TELEMETRY_CORE_SIZE, txSize);
    TEST_ASSERT_EQUAL_UINT32(60, payload.sleepInterval);

    input.devMode = true;
    buildTelemetryPayload(input, payload, txSize);
    TEST_ASSERT_EQUAL_UINT32(TELEMETRY_FULL_SIZE, txSize);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_sensor_marks_unset_clock_for_sync);
    RUN_TEST(test_sensor_marks_valid_clock_as_synced);
    RUN_TEST(test_sensor_uses_compact_and_full_sizes);
    return UNITY_END();
}
