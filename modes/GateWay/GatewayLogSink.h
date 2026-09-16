#pragma once

#include <Arduino.h>
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <time.h>

class GatewayLogSink {
public:
    void begin(bool sdAvailable) {
        sdReady() = sdAvailable;
        if (sdReady()) {
            SD.mkdir("/logs");
            SD.mkdir("/logs/archive");
            File active = SD.open("/logs/active.log", FILE_APPEND);
            if (active) active.close();
            else sdReady() = false;
        }
        if (!queue()) queue() = xQueueCreate(32, sizeof(LogLine));
        if (queue() && !writerStarted()) {
            writerStarted() = xTaskCreatePinnedToCore(writerTask, "gatewayLogWriter", 4096, nullptr, 1, nullptr, 0) == pdPASS;
        }
    }

    bool isSdReady() const { return sdReady(); }

    template <typename... Args>
    void write(const char* level, const char* functionName, const char* format, Args... args) {
        char message[MAX_LINE - 80];
        snprintf(message, sizeof(message), format, args...);

        char line[MAX_LINE];
        time_t now;
        time(&now);
        struct tm timeInfo;
        if (localtime_r(&now, &timeInfo)) {
            strftime(line, sizeof(line), "[%Y-%m-%d %H:%M:%S]", &timeInfo);
        } else {
            snprintf(line, sizeof(line), "[1970-01-01 00:00:00]");
        }
        snprintf(line + strlen(line), sizeof(line) - strlen(line), " [%s] [%s] \"%s\"\n", level, functionName, message);

        if (queue()) xQueueSend(queue(), &line, 0);
    }

private:
    static constexpr size_t MAX_LINE = 384;
    static constexpr uint32_t MAX_ACTIVE_SIZE = 1024UL * 1024UL;
    struct LogLine { char text[MAX_LINE]; };

    static QueueHandle_t& queue() { static QueueHandle_t value = nullptr; return value; }
    static bool& writerStarted() { static bool value = false; return value; }
    static bool& sdReady() { static bool value = false; return value; }
    static int& lastDay() { static int value = -1; return value; }

    static void writerTask(void*) {
        LogLine line;
        while (true) {
            if (xQueueReceive(queue(), &line, portMAX_DELAY) == pdTRUE) {
                Serial.print(line.text);
                writeToSd(line.text);
            }
        }
    }

    static void writeToSd(const char* line) {
        if (!sdReady()) return;
        if (SD.cardType() == CARD_NONE) {
            sdReady() = false;
            return;
        }
        SD.mkdir("/logs");
        SD.mkdir("/logs/archive");
        rotateIfNeeded();
        File file = SD.open("/logs/active.log", FILE_APPEND);
        if (file) {
            file.print(line);
            file.close();
        } else {
            sdReady() = false;
        }
    }

    static void rotateIfNeeded() {
        File active = SD.open("/logs/active.log", FILE_READ);
        if (!active) return;
        bool sizeExceeded = active.size() >= MAX_ACTIVE_SIZE;
        active.close();
        time_t now; time(&now);
        struct tm timeInfo;
        bool hasValidDay = now >= 1704067200UL && localtime_r(&now, &timeInfo);
        if (hasValidDay && lastDay() < 0) lastDay() = timeInfo.tm_yday;
        bool dayChanged = hasValidDay && timeInfo.tm_yday != lastDay();
        if (!sizeExceeded && !dayChanged) return;
        char archive[64];
        if (now >= 1704067200UL && localtime_r(&now, &timeInfo)) {
            strftime(archive, sizeof(archive), "/logs/archive/gateway-%Y%m%d-%H%M%S.log", &timeInfo);
        } else {
            snprintf(archive, sizeof(archive), "/logs/archive/gateway-%lu.log", millis());
        }
        if (SD.exists(archive)) SD.remove(archive);
        SD.rename("/logs/active.log", archive);
        if (hasValidDay) lastDay() = timeInfo.tm_yday;
    }
};

static GatewayLogSink gatewayLogSink;

#define GW_LOG_DEBUG(format, ...) gatewayLogSink.write("DEBUG", __func__, format, ##__VA_ARGS__)
#define GW_LOG_INFO(format, ...)  gatewayLogSink.write("INFO", __func__, format, ##__VA_ARGS__)
#define GW_LOG_WARN(format, ...)  gatewayLogSink.write("WARN", __func__, format, ##__VA_ARGS__)
#define GW_LOG_ERROR(format, ...) gatewayLogSink.write("ERROR", __func__, format, ##__VA_ARGS__)
