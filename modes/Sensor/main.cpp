#include <RadioLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <INA226_WE.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <mbedtls/aes.h>
#include <esp_system.h>
#include <LoRaHomeCommon.h>
#include <LoRaBoards.h>
#include <secrets.h>

// Pin definitions
#define DS18B20_PIN 4  // GPIO4 for DS18B20
// Use a non-strapping pin to avoid boot issues (GPIO0/2/4/12/15 are strapping pins)
#define DEV_MODE_PIN 13 // GPIO13 for dev mode toggle (pull low to enable)
// Magic word to validate RTC memory integrity
#define RTC_MAGIC_WORD 0xA1B2C3D4
#define RTC_LINK_METRICS_MAGIC 0x4C4D4554
#define SENSOR_CONFIG_VERSION 2

#if     defined(USING_SX1276)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           868.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   17
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);

#elif   defined(USING_SX1278)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           433.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   17
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1278 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);

#elif   defined(USING_SX1262)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           850.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   22
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif

SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif   defined(USING_SX1280)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           2400.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   13
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             203.125
#endif
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif  defined(USING_SX1280PA)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           2400.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   3           // PA Version power range : -18 ~ 3dBm
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             203.125
#endif
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif   defined(USING_SX1268)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           433.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   22
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif   defined(USING_LR1121)

/*
* Important: LR1121 PA Version
*
* The 2.4G version does not have a power amplifier (PA). The permissible power setting is 13dBm.
*
* If it is a version with a built-in PA, please do not exceed 0dBm in the maximum power setting.
* This is because a power amplifier has been added to the RF front-end; setting it to 0dBm will achieve an output power of 22dBm.
* Setting it to more than 1dBm may damage the PA.
*
* */

#define CONFIG_RADIO_FREQ           2450.0
#define CONFIG_RADIO_OUTPUT_POWER   LILYGO_RADIO_2G4_TX_POWER_LIMIT
#define CONFIG_RADIO_BW             125.0

// The maximum power of LR1121 Sub 1G band can only be set to 22 dBm
// #define CONFIG_RADIO_FREQ           868.0
// #define CONFIG_RADIO_OUTPUT_POWER   22
// #define CONFIG_RADIO_BW             125.0

LR1121 radio = new Module(RADIO_CS_PIN, RADIO_DIO9_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#ifdef USING_LR1121PA
// LR1121 Version PA RF switch table
static const uint32_t pa_version_rf_switch_dio_pins[] = {
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6, RADIOLIB_LR11X0_DIO7, RADIOLIB_LR11X0_DIO8, RADIOLIB_NC
};

static const Module::RfSwitchMode_t high_freq_switch_table[] = {
    // mode                  DIO5  DIO6 DIO7 DIO8
    { LR11x0::MODE_STBY,   { LOW,  LOW, LOW, LOW} },
    { LR11x0::MODE_TX,     { LOW,  LOW, LOW, HIGH} },
    { LR11x0::MODE_RX,     { LOW,  LOW, HIGH, LOW} },
    { LR11x0::MODE_TX_HP,  { LOW,  LOW, HIGH, LOW} },
    { LR11x0::MODE_TX_HF,  { LOW,  LOW, HIGH, LOW} },
    { LR11x0::MODE_GNSS,   { LOW,  LOW, LOW, HIGH} },
    { LR11x0::MODE_WIFI,   { LOW,  LOW, LOW, HIGH} },
    END_OF_MODE_TABLE,
};

static const Module::RfSwitchMode_t low_freq_switch_table[] = {
    // mode                  DIO5  DIO6 DIO7 DIO8
    { LR11x0::MODE_STBY,   { LOW,  LOW, LOW, LOW} },
    { LR11x0::MODE_TX,     { LOW,  HIGH, LOW, LOW} },
    { LR11x0::MODE_RX,     { HIGH, LOW, LOW, LOW} },
    { LR11x0::MODE_TX_HP,  { LOW,  HIGH, LOW, LOW} },
    { LR11x0::MODE_TX_HF,  { LOW,  LOW, LOW, LOW} },
    { LR11x0::MODE_GNSS,   { LOW,  LOW, LOW, LOW} },
    { LR11x0::MODE_WIFI,   { LOW,  LOW, LOW, LOW} },
    END_OF_MODE_TABLE,
};

#endif /*USING_LR1121PA*/
#endif /*Radio define end*/


// Sensor objects
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
INA226_WE ina226 = INA226_WE(0x40); // INA226 at default I2C address 0x40
static constexpr float INA226_SHUNT_OHMS = 0.1f;
static constexpr float INA226_MAX_CURRENT_AMPS = 1.0f;
// Change to -1.0f if the installed shunt direction should be inverted.
static constexpr float INA226_CURRENT_DIRECTION = 1.0f;


// save transmission state between loops
static int transmissionState = RADIOLIB_ERR_NONE;
// FreeRTOS semaphore to replace the busy-wait flag
static SemaphoreHandle_t radioSemaphore = NULL;
RTC_DATA_ATTR static uint16_t counter = 0;
RTC_DATA_ATTR static uint8_t wakeCycleCount = 0;
RTC_DATA_ATTR static uint32_t bootNonce = 0;
RTC_DATA_ATTR static bool lowBatteryLockout = false;

struct LinkMetrics {
  uint32_t magicWord;
  float lastSNR;
  float lastRSSI;
};

RTC_NOINIT_ATTR LinkMetrics rtcLinkMetrics;
static String payload;
static uint32_t lastTxTime = 0;
static uint32_t lastRxTime = 0;
static uint32_t lastBeaconRxTime = 0;
static const uint8_t UPDATE_CHECK_MAX_INTERVAL_SECONDS = 60;
static const uint32_t RADIO_INIT_RETRY_INTERVAL_SECONDS = 300;
static const uint32_t LOW_BATTERY_SLEEP_INTERVAL_SECONDS = 3600;
static constexpr float LOW_BATTERY_CUTOFF_VOLTS = 3.2f;
static constexpr float LOW_BATTERY_RECOVERY_VOLTS = 3.4f;
static bool ina226Initialized = false;
static const uint32_t ACTIVE_POWER_SAMPLE_INTERVAL_MS = 20;
static float activePowerSampleSumMw = 0.0f;
static uint16_t activePowerSampleCount = 0;
static float completedActivePowerMw = 0.0f;
static bool completedActivePowerValid = false;
// Transmission details
static String deviceId;
static int screenNum = -1;
static int msgOffset = 0;
// Dev mode flag
static bool devMode = false;
// Manual flag to enable serial output in Operation mode for testing. Requires reflash.
static bool serialEnabled = false; 
static unsigned long bootTime = 0;
static bool bootTimeSet = false;

// Adds a timestamp to all Sensor diagnostic output without changing call sites.
// UTC is used after a gateway time sync; otherwise elapsed boot time is shown.
class TimestampedSerial {
public:
    explicit TimestampedSerial(HardwareSerial& serialPort) : serial(serialPort) {}

    void begin(unsigned long baud) { serial.begin(baud); }
    void flush() { serial.flush(); }

    template <typename T>
    size_t print(const T& value) {
        writePrefix();
        return serial.print(value);
    }

    template <typename T>
    size_t println(const T& value) {
        writePrefix();
        return serial.println(value);
    }

    size_t println() {
        writePrefix();
        return serial.println();
    }

    int printf(const char* format, ...) {
        char message[256];
        va_list args;
        va_start(args, format);
        int length = vsnprintf(message, sizeof(message), format, args);
        va_end(args);
        writePrefix();
        serial.print(message);
        return length;
    }

private:
    HardwareSerial& serial;

    void writePrefix() {
        time_t now;
        time(&now);
        if (isClockValidSince(now, millis() / 1000)) {
            char timestamp[24];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
            serial.printf("[%s] ", timestamp);
        } else {
            serial.printf("[+%lus] ", millis() / 1000);
        }
    }
};

TimestampedSerial sensorSerial(Serial);
#define Serial sensorSerial

// Sensor Data structure
struct SensorData {
  float temperature;
  float batteryVoltage;
  float batteryCurrent;
  float batteryPower;
  uint8_t batteryPercent;
  float cpuTemp;
  uint32_t freeRam;
  float lastSNR;
  float lastRSSI;
  uint32_t timestamp;
} sensorData;

// Configuration structure
struct Config {
  uint32_t magicWord;
  uint8_t configVersion;
  uint32_t sleepInterval; // seconds
  uint8_t isDevMode; // Remote dev mode flag (0=false, 1=true)
  int8_t txPower; // Gateway-configured transmit power in dBm
} config = {RTC_MAGIC_WORD, SENSOR_CONFIG_VERSION, 20, 0, CONFIG_RADIO_OUTPUT_POWER};

// RTC memory for config persistence
RTC_NOINIT_ATTR Config rtcConfig; // NOINIT ensures it survives SW_CPU_RESET (ESP.restart)

// Function Prototypes
void readSensors();
void transmitData();
void enterDeepSleep(uint32_t sleepSeconds = 0);
void wakeCycle();
void checkForUpdates();
bool waitForUpdateBeacon();
bool listenForConfig();
void drawMain();
void flushSerialOutput();
void beginActivePowerSampling();
void sampleActivePower();
void finishActivePowerSampling();
bool waitForRadioEvent(uint32_t timeoutMs);

// Check if current time reflects true NTP sync (not just boot epoch + drift)
bool isClockValid(time_t currentTime) {
    if (!bootTimeSet) {
        bootTime = millis();
        bootTimeSet = true;
    }

    unsigned long uptimeSeconds = (millis() - bootTime) / 1000;
    bool valid = isClockValidSince(currentTime, uptimeSeconds);
    if (serialEnabled) {
        Serial.println(valid ? "Clock is valid" : "Clock is not valid - likely still at boot epoch");
    }
    return valid;
}

void flushSerialOutput() {
    if (!serialEnabled) {
        return;
    }

    Serial.flush();
    delay(20);
}


// Callback function for LoRa hardware interrupts.
// IMPORTANT: This function MUST be 'void' type and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (radioSemaphore != NULL) {
        xSemaphoreGiveFromISR(radioSemaphore, &xHigherPriorityTaskWoken);
    }
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// Standard Arduino setup function. Initializes hardware, sensors, and radio.
// Used in: Both Operation and Dev modes
void setup()
{
    // Initialize the FreeRTOS semaphore
    radioSemaphore = xSemaphoreCreateBinary();

    // RTC memory preserves this across deep sleep; a cold boot gets a new
    // public session value so AES-CTR streams cannot repeat after counter reset.
    if (bootNonce == 0) {
        bootNonce = esp_random();
        if (bootNonce == 0) bootNonce = 1;
    }

    if (rtcLinkMetrics.magicWord != RTC_LINK_METRICS_MAGIC) {
        rtcLinkMetrics.magicWord = RTC_LINK_METRICS_MAGIC;
        rtcLinkMetrics.lastSNR = 0;
        rtcLinkMetrics.lastRSSI = 0;
    }

    // Early evaluate devMode to enable serial immediately if needed
    pinMode(DEV_MODE_PIN, INPUT_PULLUP);
    if (digitalRead(DEV_MODE_PIN) == LOW ||
        (rtcConfig.configVersion == SENSOR_CONFIG_VERSION && rtcConfig.isDevMode != 0)) {
        serialEnabled = true;
    }

    if (serialEnabled) {
        Serial.begin(115200);
    }

    // Load config from RTC memory first to evaluate remote dev mode
    config = rtcConfig;


    
    // Validate RTC memory using the magic word and basic bounds checking
    if (config.magicWord != RTC_MAGIC_WORD || config.configVersion != SENSOR_CONFIG_VERSION ||
        config.sleepInterval < 10 || config.sleepInterval > 86400) {
        config.magicWord = RTC_MAGIC_WORD;
        config.configVersion = SENSOR_CONFIG_VERSION;
        // TO DO - configure defauls centrally
        config.sleepInterval = 60;
        config.isDevMode = 0;
        config.txPower = CONFIG_RADIO_OUTPUT_POWER;
        rtcConfig = config; // Initialize RTC memory with defaults on cold boot
        if (serialEnabled) {
            Serial.printf("invalid rtcConfig, reinitialising to defaults: %u sleep=%u, devMode=%d, txPower=%ddBm\n",
                          rtcConfig.magicWord, rtcConfig.sleepInterval, rtcConfig.isDevMode, rtcConfig.txPower);
        }
    } else{
            if (serialEnabled) {
                Serial.printf("Boot rtcConfig is valid : %u sleep=%u, devMode=%d, txPower=%ddBm\n",
                                rtcConfig.magicWord, rtcConfig.sleepInterval, rtcConfig.isDevMode, rtcConfig.txPower);
            }
    }



    // Check dev mode (hardware pin OR remote config)
    devMode = (digitalRead(DEV_MODE_PIN) == LOW) || (config.isDevMode != 0);

    if (devMode) {
        setupBoards(); // Initialize all peripherals for dev mode
        if (serialEnabled) Serial.println("Dev/Debug mode");
    } else {
        if (serialEnabled) Serial.println("Operation mode");
        // In OP mode, disable all non-essential peripherals for max power saving
        // Disable WiFi and Bluetooth
        WiFi.mode(WIFI_OFF);
        btStop();
        
        // Initialize I2C only for sensors
        Wire.begin(21, 22); // SDA, SCL

        // Initialize SPI for radio
        SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
        
        // Set radio pins
        pinMode(18, OUTPUT); // RADIO_CS_PIN
        digitalWrite(18, HIGH);
        pinMode(23, OUTPUT); // RADIO_RST_PIN
        digitalWrite(23, HIGH);
        pinMode(32, INPUT); // RADIO_DIO2_PIN
    }
    
    // Initialize LED
    pinMode(BOARD_LED, OUTPUT);
    digitalWrite(BOARD_LED, !LED_ON);

    // Initialize sensors
    sensors.begin();
    sensors.setResolution(9); // 0.5 C steps, displayed with one decimal; 93.75 ms max conversion
    ina226Initialized = ina226.init(); // Initialize INA226
    if (ina226Initialized) {
        ina226.setResistorRange(INA226_SHUNT_OHMS, INA226_MAX_CURRENT_AMPS);
    } else if (serialEnabled) {
        Serial.println("INA226 init failed; power readings may be invalid.");
    }

    // Radio setup (same as before, but only if not sleeping)
    int state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        if (serialEnabled) {
            Serial.print(F("Radio init failed: "));
            Serial.println(state);
        }
        enterDeepSleep(RADIO_INIT_RETRY_INTERVAL_SECONDS);
        return;
    }
    // Set radio parameters (same as before)
    radio.setFrequency(CONFIG_RADIO_FREQ);
    radio.setBandwidth(CONFIG_RADIO_BW);
    radio.setSpreadingFactor(9);  // SF9 is a good balance of range and speed
    radio.setCodingRate(5);       // CR 4/5 is standard
    radio.setSyncWord(0xAB);
    state = radio.setOutputPower(config.txPower);
    if (state != RADIOLIB_ERR_NONE) {
        if (serialEnabled) {
            Serial.printf("Configured TX power %ddBm rejected (state %d); using firmware default %ddBm\n",
                          config.txPower, state, CONFIG_RADIO_OUTPUT_POWER);
        }
        config.txPower = CONFIG_RADIO_OUTPUT_POWER;
        rtcConfig = config;
        radio.setOutputPower(config.txPower);
    }
    radio.setCRC(true);           // Enable CRC for data integrity

    // Set hardware interrupt callbacks for both RX and TX
    radio.setPacketSentAction(setFlag);
    radio.setPacketReceivedAction(setFlag);

    // Display operation mode at startup screen
    #ifdef HAS_DISPLAY
    if (devMode && disp) {  //only in dev mode
        // Unique device identity (use MAC)
        uint64_t mac = ESP.getEfuseMac();
        char idBuf[17];
        sprintf(idBuf, "%08X%08X", (uint32_t)(mac >> 32), (uint32_t)mac);
        deviceId = String(idBuf);

        const char *modeText = "DEV MODE";
        disp->clearBuffer();
        disp->setFont(u8g2_font_pxplusibmvga9_mr);
        int16_t x = (disp->getDisplayWidth() - disp->getUTF8Width(modeText)) / 2;
        disp->drawStr(x, 30, modeText);
        disp->sendBuffer();
        delay(2000);
        disp->clearBuffer();
        disp->sendBuffer();
    }
    #endif

    // In Op Mode, perform one cycle and go to deep sleep
    // In Dev Mode, perform the first cycle, then continue in loop().
    wakeCycle();

    if (devMode) {
        drawMain(); 
    } else {
        enterDeepSleep();
    }
}

// Standard Arduino loop function.
// Used ONLY in Dev mode (Simulates the device op lifecycle with serial logging and display updates).
void loop()
{
    if (devMode) {
       
        Serial.println("[DEV] --- Sleep ---");
        Serial.printf("Entering deep sleep for %d seconds\n", config.sleepInterval);

        if (disp) {
            disp->clearBuffer();
            disp->sendBuffer();
        }
        delay(config.sleepInterval * 1000); // Simulate deep sleep from cfg file

        // run the operating cycle
        wakeCycle();
        
        // display data from the cycle
        Serial.println("[DEV] --- Display ---");
        // Rotate through all 4 screens
        for (int i = 0; i < 4; i++) {
            drawMain();
            delay(2000); // Show each screen for 2s
        }
    } else {
        
    }
}

// Encrypts or decrypts a payload in place using AES-128-CTR
void cryptPayload(uint8_t* data, size_t length, const uint8_t* mac, uint32_t bootNonceValue, uint16_t msgCount) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, AES_NETWORK_KEY, 128); // 128-bit AES
    
    // Bind the CTR nonce to this sensor and boot session. The public MAC and
    // bootNonce prevent different sensors or cold boots from reusing a stream.
    uint8_t ctrNonce[16];
    memcpy(ctrNonce, mac, 6);
    memcpy(ctrNonce + 6, &bootNonceValue, sizeof(bootNonceValue));
    ctrNonce[10] = msgCount & 0xFF;
    ctrNonce[11] = (msgCount >> 8) & 0xFF;
    memset(ctrNonce + 12, 0, 4);
    
    uint8_t stream_block[16] = {0};
    size_t nc_off = 0;
    mbedtls_aes_crypt_ctr(&aes, length, &nc_off, ctrNonce, stream_block, data, data);
    mbedtls_aes_free(&aes);
}

// Converts a given voltage to an estimated percentage (0-100%) for a typical 18650 Li-Ion battery #6
// Used in: Both Operation and Dev modes
uint8_t getBatteryPercentage(float voltage) {
    int voltage_mv = voltage * 1000;
    // Voltage lookup table for 0%, 10%, 20%, ..., 100%
    const static int table[11] = {
        3000, 3650, 3700, 3740, 3760, 3795,
        3840, 3910, 3980, 4070, 4150
    };
    if (voltage_mv < table[0]) return 0;
    for (int i = 1; i < 11; i++) {
        if (voltage_mv < table[i]) {
            return i * 10 - (10 * (table[i] - voltage_mv)) / (table[i] - table[i - 1]);
        }
    }
    return 100;
}

// Starts a development-mode measurement window for LoRa TX and active RX power.
void beginActivePowerSampling() {
    activePowerSampleSumMw = 0.0f;
    activePowerSampleCount = 0;
}

// Samples signed battery power. V * mA yields mW.
void sampleActivePower() {
    if (!devMode || !ina226Initialized) {
        return;
    }

    float voltage = ina226.getBusVoltage_V();
    float current = ina226.getCurrent_mA() * INA226_CURRENT_DIRECTION;
    activePowerSampleSumMw += voltage * current;
    activePowerSampleCount++;
}

// Retains the completed average for the next development telemetry packet.
void finishActivePowerSampling() {
    if (devMode && activePowerSampleCount > 0) {
        completedActivePowerMw = activePowerSampleSumMw / activePowerSampleCount;
        completedActivePowerValid = true;
    }
}

// Waits for a radio interrupt while periodically sampling power in development mode.
bool waitForRadioEvent(uint32_t timeoutMs) {
    uint32_t startTime = millis();

    while (millis() - startTime < timeoutMs) {
        uint32_t elapsed = millis() - startTime;
        uint32_t remaining = timeoutMs - elapsed;
        uint32_t waitMs = (devMode && ina226Initialized && remaining > ACTIVE_POWER_SAMPLE_INTERVAL_MS)
                            ? ACTIVE_POWER_SAMPLE_INTERVAL_MS
                            : remaining;

        if (xSemaphoreTake(radioSemaphore, pdMS_TO_TICKS(waitMs)) == pdTRUE) {
            sampleActivePower();
            return true;
        }
        sampleActivePower();
    }

    return false;
}

// single operational cycle: read, transmit, and check for updates.
// Used in: Both Operation and Dev modes
void wakeCycle() {

    if (serialEnabled) Serial.println("\n--- Wake, Read Sensors ---");
    readSensors();

    if (ina226Initialized) {
        bool batteryStillLow = lowBatteryLockout
                               ? sensorData.batteryVoltage < LOW_BATTERY_RECOVERY_VOLTS
                               : sensorData.batteryVoltage <= LOW_BATTERY_CUTOFF_VOLTS;
        if (batteryStillLow) {
            lowBatteryLockout = true;
            if (serialEnabled) {
                Serial.printf("Battery %.2fV below safe operating threshold; suppressing radio and sleeping.\n",
                              sensorData.batteryVoltage);
            }
            enterDeepSleep(LOW_BATTERY_SLEEP_INTERVAL_SECONDS);
            return;
        }
        lowBatteryLockout = false;
    }

    beginActivePowerSampling();
    
    if (serialEnabled) Serial.println("--- Transmit ---");
    transmitData();


    checkForUpdates();
    finishActivePowerSampling();
}   

// Reads data from connected sensors (DS18B20, INA226) and internal ESP32 metrics (CPU temp, RAM).
// Used in: Both Operation and Dev modes
void readSensors()
{   
    // Preserve the most recent valid config-downlink metrics across deep sleep and soft resets.
    sensorData.lastSNR = rtcLinkMetrics.lastSNR;
    sensorData.lastRSSI = rtcLinkMetrics.lastRSSI;
    
    // Read DS18B20 temperature
    sensors.requestTemperatures();
    sensorData.temperature = sensors.getTempCByIndex(0);

    // Read INA226 data. Power is calculated from signed current so charge and
    // discharge direction is preserved instead of using the unsigned power register.
    if (ina226Initialized) {
        sensorData.batteryVoltage = ina226.getBusVoltage_V();
        sensorData.batteryCurrent = ina226.getCurrent_mA() * INA226_CURRENT_DIRECTION;
        float idlePowerMw = sensorData.batteryVoltage * sensorData.batteryCurrent;
        sensorData.batteryPower = (devMode && completedActivePowerValid)
                                  ? completedActivePowerMw
                                  : idlePowerMw;
        completedActivePowerValid = false;
        sensorData.batteryPercent = getBatteryPercentage(sensorData.batteryVoltage);
    } else {
        sensorData.batteryVoltage = 0.0f;
        sensorData.batteryCurrent = 0.0f;
        sensorData.batteryPower = 0.0f;
        sensorData.batteryPercent = 0;
    }

    // Read ESP32 internals
    sensorData.cpuTemp = temperatureRead();
    sensorData.freeRam = ESP.getFreeHeap() / 1024;

    // Get current time (ESP32 RTC survives deep sleep, drift is fixed on config update)
    time_t now;
    time(&now);
    sensorData.timestamp = (uint32_t)now;
    if (serialEnabled) {
        char timeStr[20];
        time_t ts = sensorData.timestamp;
        if (isClockValid(ts)) {
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", gmtime(&ts));
        } else {
            strncpy(timeStr, "UNSYNCED", sizeof(timeStr));
            timeStr[sizeof(timeStr) - 1] = '\0';
        }

        Serial.printf("T:%.1fC V:%.2fV(%d%%) I:%.1fmA P:%.1fmW | CPU:%.1fC RAM:%uKB | SNR:%.1f RSSI:%.0f | TS:%s\n",
                      sensorData.temperature, sensorData.batteryVoltage, sensorData.batteryPercent,
                      sensorData.batteryCurrent, sensorData.batteryPower, sensorData.cpuTemp,
                      sensorData.freeRam, sensorData.lastSNR, sensorData.lastRSSI,
                      timeStr);
    }
}

// Constructs the telemetry payload string and transmits it via the LoRa radio.
// Used in: Both Operation and Dev modes
void transmitData()
{
    // Prepare binary payload
    TelemetryPayload txPayload;
    esp_efuse_mac_get_default(txPayload.mac);
    txPayload.bootNonce = bootNonce;
    txPayload.msgCount = ++counter;
    txPayload.temperature = (int16_t)(sensorData.temperature * 100); 
    txPayload.battVoltage = (uint16_t)(sensorData.batteryVoltage * 1000);
    txPayload.battPercent = sensorData.batteryPercent;
    txPayload.isDevMode = (uint8_t)devMode;
    txPayload.needsTimeSync = isClockValid((time_t)sensorData.timestamp) ? 0 : 1;
    txPayload.sleepInterval = config.sleepInterval;
    txPayload.txPower = config.txPower;
    size_t txSize = TELEMETRY_CORE_SIZE; // Core payload size

    if (devMode) {
        txPayload.battCurrent = (int16_t)sensorData.batteryCurrent;
        txPayload.battPower = (int16_t)sensorData.batteryPower;
        txPayload.freeRam = (uint16_t)sensorData.freeRam;
        txPayload.cpuTemp = (int8_t)sensorData.cpuTemp;
        txPayload.lastSNR = (int8_t)sensorData.lastSNR;
        txPayload.lastRSSI = (int16_t)sensorData.lastRSSI;
        txSize = sizeof(TelemetryPayload); // 31 bytes full size
    }

    if (serialEnabled) {
        // We generate the debug string before encryption so you can still read it on the OLED
        payload = "";
        uint8_t* ptr = (uint8_t*)&txPayload;
        for(size_t i = 0; i < txSize; i++) {
            char buf[3];
            sprintf(buf, "%02X", ptr[i]);
            payload += buf;
        }
    }

    // Encrypt only after the public nonce material (MAC, boot nonce, counter).
    size_t encryptedLength = txSize - 12;
    uint8_t* dataPtr = ((uint8_t*)&txPayload) + 12;
    cryptPayload(dataPtr, encryptedLength, txPayload.mac, txPayload.bootNonce, txPayload.msgCount);

    // Turn LED on during transmission
    digitalWrite(BOARD_LED, LED_ON);

    // Clear any pending semaphore state
    xSemaphoreTake(radioSemaphore, 0);

    // Transmit the raw binary struct directly
    transmissionState = radio.startTransmit((uint8_t*)&txPayload, txSize);

    if (transmissionState != RADIOLIB_ERR_NONE) {
        if (serialEnabled) Serial.printf("Failed to start TX (state %d)\n", transmissionState);
        digitalWrite(BOARD_LED, !LED_ON);
        radio.standby();
        return;
    }
    sampleActivePower();

    if (serialEnabled) {
        Serial.printf("Transmitting binary payload (%d bytes): %s\n", txSize, payload.c_str());
    }

    // Wait for transmission to complete (with 5 second timeout) using FreeRTOS Block state
    uint32_t startWait = millis();
    if (!waitForRadioEvent(5000)) {
        if (serialEnabled) Serial.println("Warning: TX timeout!");
    }
    lastTxTime = millis() - startWait;

    // Actively put the radio to standby after TX finishes
    radio.standby();

    // Turn LED off after transmission
    digitalWrite(BOARD_LED, !LED_ON);

    if (serialEnabled) {
        Serial.printf("Transmission complete. Airtime: %lu ms\n", lastTxTime);
    }
    
}

// Checks for pending gateway updates at least every 60 seconds or when the clock is invalid.
// Used in: Both Operation and Dev modes
void checkForUpdates() {
    wakeCycleCount++;
    uint8_t cyclesBetweenChecks = 1;
    if (config.sleepInterval > 0) {
        cyclesBetweenChecks = UPDATE_CHECK_MAX_INTERVAL_SECONDS / config.sleepInterval;
        if (cyclesBetweenChecks == 0) cyclesBetweenChecks = 1;
    }
    if (serialEnabled) {
        Serial.printf("Wakecycle: %d/%d\n", wakeCycleCount, cyclesBetweenChecks);
    }

    bool updateIntervalReached = wakeCycleCount >= cyclesBetweenChecks;
    bool needsUpdateCheck = updateIntervalReached || !isClockValid(sensorData.timestamp);
    if (needsUpdateCheck) {
        // Schedule the next periodic check even when no update beacon is pending.
        wakeCycleCount = 0;
        if (serialEnabled) {
            Serial.printf("Checking for updates. Reason: %s\n", updateIntervalReached ? "update interval reached" : "Time not synced");
        }
        if (waitForUpdateBeacon()) {
            if (listenForConfig()) {
                wakeCycleCount = 0; // Keep the periodic counter reset after successful config reception
            } else {
                if (serialEnabled) {
                    Serial.println("Failed to Receive Config after beacon.");
                }
            }
        }
    }
}

// Briefly checks if the gateway has a pending configuration update for this wake cycle.
// Used in: Both Operation and Dev modes
bool waitForUpdateBeacon()
{
    xSemaphoreTake(radioSemaphore, 0); // Clear semaphore
    radio.startReceive();// Start non-blocking background reception
    sampleActivePower();
    unsigned long startTime = millis();
    // Allow the gateway enough scheduling headroom to switch from RX to TX.
    uint32_t beaconAirtimeMs = (radio.getTimeOnAir(1) / 1000) + 250;
    while (millis() - startTime < beaconAirtimeMs) {
        uint32_t elapsed = millis() - startTime;
        uint32_t remaining = beaconAirtimeMs > elapsed ? beaconAirtimeMs - elapsed : 0;
        if (remaining == 0) break;

        if (waitForRadioEvent(remaining)) {
            int numBytes = radio.getPacketLength();
            uint8_t rxBuffer[256];
            int state = radio.readData(rxBuffer, numBytes);

            if (state == RADIOLIB_ERR_NONE) {
                if (numBytes == 1 && rxBuffer[0] == CONFIG_UPDATE_PENDING_FLAG) {
                    lastBeaconRxTime = millis() - startTime;
                    if (serialEnabled) {
                        Serial.printf("Update beacon received after %lu ms (window: %lu ms)\n", lastBeaconRxTime, beaconAirtimeMs);
                    }
                    return true;
                } else if (serialEnabled) {
                    Serial.printf("Received %d bytes (expected 1). Retrying...\n", numBytes);
                }
            } else if (serialEnabled) {
                Serial.printf("RX Error: %d. Retrying...\n", state);
            }
            radio.startReceive(); 
            sampleActivePower();
        }
        if (serialEnabled) {
                    Serial.printf("timeout while trying to find beacon.");
                }
    }
    radio.standby();
    lastBeaconRxTime = millis() - startTime;
    if (serialEnabled) {
        Serial.printf("No update beacon found. Short RX window: %lu ms (was %lu ms)\n", lastBeaconRxTime, beaconAirtimeMs);
    }
    return false;
}

// Listens for incoming LoRa configuration packets from the Gateway for 5 seconds.
// Returns true if config was successfully received and applied, false otherwise.
// Used in: Both Operation and Dev modes
bool listenForConfig()
{
    xSemaphoreTake(radioSemaphore, 0); // Clear semaphore
    radio.startReceive(); // Start non-blocking background reception
    sampleActivePower();
    unsigned long startTime = millis();
    bool configReceived = false;
    bool needsRestart = false;

    while (millis() - startTime < 5000) { // Listen for 5 seconds
        uint32_t elapsed = millis() - startTime;
        uint32_t remaining = 5000 > elapsed ? 5000 - elapsed : 0;
        if (remaining == 0) break;

        if (waitForRadioEvent(remaining)) {
            // Verify packet length matches our expected Config struct size
            if (radio.getPacketLength() == sizeof(ConfigPayload)) {
                ConfigPayload rxConfig;
                int state = radio.readData((uint8_t*)&rxConfig, sizeof(ConfigPayload));

                if (state == RADIOLIB_ERR_NONE) {
                    float receivedSNR = radio.getSNR();
                    float receivedRSSI = radio.getRSSI();

                    if (serialEnabled) {
                        uint32_t packetToA = radio.getTimeOnAir(sizeof(ConfigPayload)) / 1000; // Returns microseconds, convert to ms
                        Serial.printf("Received config packet. Packet ToA: %lu ms\n", packetToA);
                    }

                    // Verify header to ensure it's actually our config packet
                    if (rxConfig.header[0] == 'C' && rxConfig.header[1] == 'F') {

                        if (rxConfig.configVersion != SENSOR_CONFIG_VERSION) {
                            if (serialEnabled) Serial.printf("Unsupported config version %u.\n", rxConfig.configVersion);
                            break;
                        }

                        // Verify this config packet is meant for this specific device
                        uint8_t myMac[6];
                        esp_efuse_mac_get_default(myMac);
                        uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

                        if (memcmp(rxConfig.targetMac, myMac, 6) != 0 && memcmp(rxConfig.targetMac, broadcastMac, 6) != 0) {
                            if (serialEnabled) Serial.println("Config not for this node. Terminating RX.");
                            break; // Not meant for this node, terminate early
                        }

                        // Verify network key to prevent spoofing attacks
                        if (rxConfig.networkKey != NETWORK_KEY) {
                            if (serialEnabled) Serial.println("Invalid network key. Terminating RX.");
                            break; // Invalid key, terminate early
                        }

                        sensorData.lastSNR = receivedSNR;
                        sensorData.lastRSSI = receivedRSSI;
                        rtcLinkMetrics.lastSNR = receivedSNR;
                        rtcLinkMetrics.lastRSSI = receivedRSSI;

                        int txPowerState = radio.setOutputPower(rxConfig.txPower);
                        if (txPowerState != RADIOLIB_ERR_NONE) {
                            if (serialEnabled) {
                                Serial.printf("Rejected config TX power %ddBm (state %d).\n", rxConfig.txPower, txPowerState);
                            }
                            break;
                        }

                        bool txPowerChanged = config.txPower != rxConfig.txPower;
                        config.magicWord = RTC_MAGIC_WORD;
                        config.configVersion = SENSOR_CONFIG_VERSION;
                        config.sleepInterval = rxConfig.sleepInterval;
                        config.isDevMode = (rxConfig.isDevMode > 0);
                        config.txPower = rxConfig.txPower;

                        if (rxConfig.timeOffset > 0) {
                            uint32_t newTime = rxConfig.timeOffset + CUSTOM_EPOCH;
                            time_t now;
                            time(&now);

                            // Sync if the local RTC is unset or has drifted by more than 2 seconds.
                            if (!isClockValid(now) || hasClockDrift(now, (time_t)newTime, 2.0)) {
                                if (serialEnabled) {
                                    Serial.printf("Time drift detected! Old: %lu, New: %lu. Syncing...\n", (unsigned long)now, (unsigned long)newTime);
                                }
                                // Sync the internal ESP32 RTC to fix time drift
                                struct timeval tv;
                                tv.tv_sec = newTime;
                                tv.tv_usec = 0;
                                settimeofday(&tv, NULL);
                            }
                        }

                        if (serialEnabled) {
                            Serial.printf("received config: sleep=%u, devMode=%d, txPower=%ddBm\n",
                                              rxConfig.sleepInterval, rxConfig.isDevMode, rxConfig.txPower);
                        }

                        rtcConfig = config; // Save to RTC
                        if (serialEnabled) {
                                Serial.printf("Config applied: sleep=%u, devMode=%d, txPower=%ddBm\n",
                                               rtcConfig.sleepInterval, rtcConfig.isDevMode, rtcConfig.txPower);
                            }

                        bool cfgMode = (digitalRead(DEV_MODE_PIN) == LOW) || (config.isDevMode != 0);
                        if (devMode != cfgMode || txPowerChanged) {
                            needsRestart = true;
                            if (serialEnabled) {
                                Serial.printf("Soft restart triggered to apply %s mode and TX power.", (rxConfig.isDevMode ==1 ) ? "DEV" : "OP");
                            }
                        } 
                        configReceived = true;
                        wakeCycleCount = 0; // Reset wakecycle

                        break; // Successfully received and applied config, exit 5s RX window early
                    } else {
                        if (serialEnabled) Serial.println("Invalid config header. Terminating RX.");
                        break;
                    }
                } else {
                    if (serialEnabled) Serial.printf("Failed to read config packet (state %d). Terminating RX.\n", state);
                    break;
                }
            } else {
                if (serialEnabled) Serial.printf("stateReceived unexpected packet length (%d). Terminating RX.\n", sizeof(ConfigPayload));
                break;
            }
        }
    }
    radio.standby();

    lastRxTime = millis() - startTime; // Record the total time the radio was consuming power in RX mode
    if (serialEnabled) {
        Serial.printf("RX Window closed. Total RX power-on time: %lu ms. Config %s\n",
                     lastRxTime, configReceived ? "received" : "NOT received");
    }
    

    if (needsRestart) {
        if (serialEnabled) {
            Serial.println("Mode switched via remote config! Restarting device..");
        }
        flushSerialOutput();
        ESP.restart(); // Soft reset
    }

    return configReceived;
}

// Handles sleep timing, conditionally triggering the configuration listener, and deep sleeping the ESP32.
// Used in: Both Operation and Dev modes (skips actual esp_deep_sleep_start in Dev mode)
void enterDeepSleep(uint32_t sleepSeconds)
{
    uint32_t requestedSleepSeconds = sleepSeconds > 0 ? sleepSeconds : config.sleepInterval;

    if (serialEnabled) {
        Serial.printf("Entering deep sleep for %u seconds\n", requestedSleepSeconds);
    }

    // Attempt this even after an initialization failure so a partially responsive
    // external radio is not left in standby while the ESP32 sleeps.
    int16_t radioSleepState = radio.sleep();
    if (radioSleepState != RADIOLIB_ERR_NONE && serialEnabled) {
        Serial.printf("Radio sleep failed: %d\n", radioSleepState);
    }

    if (ina226Initialized) {
        ina226.setMeasureMode(INA226_POWER_DOWN);
    }

    if (serialEnabled) {
        flushSerialOutput();
    }

    // Configure wake up timer
    esp_sleep_enable_timer_wakeup(requestedSleepSeconds * 1000000ULL); // microseconds

    // Enter deep sleep
    esp_deep_sleep_start();
}

// Renders telemetry and device data to the OLED display. Cycles through 4 different informational screens.
// Used in: ONLY Dev mode
void drawMain()
{
    if (devMode) {
        Serial.println("drawMain called");
        if (disp) {
            Serial.println("disp found, drawing...");
        } else {
            Serial.println("disp not found, cannot draw");
        }
    }
    if (devMode && disp) {
        screenNum = (screenNum + 1) % 4; // Cycle through 4 screens
        disp->clearBuffer();
        disp->drawRFrame(0, 0, 128, 64, 5);
        disp->setFont(u8g2_font_pxplusibmvga8_mr);

        switch (screenNum) {
            case 0: // Primary Sensors
                disp->setCursor(5, 15);
                disp->printf("TSense: %.1fC", sensorData.temperature);
                disp->setCursor(5, 30);
                disp->printf("TCPU: %.1fC", sensorData.cpuTemp);
                disp->setCursor(5, 45);
                disp->printf("VCC: %.2fV", sensorData.batteryVoltage);
                disp->setCursor(5, 60);
                disp->printf("Batt: %d%%", sensorData.batteryPercent);
                break;

            case 1: // Power Details
                disp->setCursor(5, 15);
                disp->printf("I: %+.0fmA", sensorData.batteryCurrent);
                disp->setCursor(5, 30);
                disp->printf("P: %+.0fmW", sensorData.batteryPower);
                disp->setCursor(5, 45);
                disp->printf("RAM: %u KB", sensorData.freeRam);
                break;

            case 2: // LoRa & System
                disp->setCursor(5, 15);
                disp->printf("SNR:%.1f R:%.0f", sensorData.lastSNR, sensorData.lastRSSI);
                disp->setCursor(5, 30);
                disp->printf("TX Pwr: %ddBm", config.txPower);
                disp->setCursor(5, 45);
                disp->printf("TX Time: %lu ms", lastTxTime);
                disp->setCursor(5, 60);
                disp->printf("RX Time: %lu ms", lastRxTime);
                break;

            case 3: // IDs & Payload
                disp->setCursor(5, 15);
                disp->printf("ID: %.10s", deviceId.c_str());
                disp->setCursor(5, 30);
                disp->printf("Cnt:%u", counter);
                disp->setCursor(5, 45);
                disp->printf("Sleep: %d s", config.sleepInterval);
                
                // Scrolling payload preview
                if (payload.length() > 18) { // Approx 18 chars fit
                    msgOffset = (msgOffset + 1) % (payload.length() - 18);
                } else {
                    msgOffset = 0;
                }
                disp->setCursor(0, 60);
                disp->printf(">%.18s", payload.c_str() + msgOffset);
                break;
        }
        disp->sendBuffer();
    }
}
