#include <Arduino.h>
#include <RadioLib.h>
#include <LoRaBoards.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <LoRaHomeCommon.h>
#include <SD.h>
#include <SPI.h>
#include <Crypto.h>
#include <AES.h>
#include <Elog.h>
#include <uptime.h>
#include <secrets.h> // WiFi and MQTT credentials

// ELog configuration
#define MYLOG 0  // Gateway log ID
// NTP Configuration
#define TZ_INFO "EET-2EEST,M3.5.0/3,M10.5.0/4" // Europe/Athens
// MQTT Configuration
// Creds moved to secrets.h
#define MQTT_PORT 8883              // Default port for MQTT over TLS
#define MQTT_TOPIC_PREFIX "lorahome" 

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

// FreeRTOS semaphore to replace the busy-wait flag
static SemaphoreHandle_t radioSemaphore = NULL;
static String rssi = "0dBm";
static String snr = "0dB";
// Display variables
static float sensorTemp = 0.0;
static float sensorVcc = 0.0;
static uint8_t sensorBatt = 0;
static String sensorMac = "Wait...";
static uint16_t msgCount = 0;
static uint8_t sensorIsDevMode = 0;
static bool sensorNeedsTimeSync = false;
static uint32_t sensorSleepInterval = 0;
static bool sensorHasTelemetry = false;
static int16_t sensorBattCurrent = 0;
static int16_t sensorBattPower = 0;
static uint16_t sensorFreeRam = 0;
static int8_t sensorCpuTemp = 0;
static int8_t sensorTxPower = 0;
static int8_t sensorSNR = 0;
static int16_t sensorRSSI = 0;
static uint32_t lastDisplayUpdate = 0;
static uint8_t screenNum = 0;
static bool hasValidRtcTime = false;
static long lastMqttReconnectAttempt = 0;
static long lastWifiReconnectAttempt = 0;
// MQTT connection state tracking for reconnection logic
static bool mqttConnected = false;
static const unsigned long MAX_RECONNECT_ATTEMPTS = 5;   // Maximum retry attempts
static const unsigned long RECONNECT_DELAY_MS = 1000;    // Delay between retries (1 second)
static const unsigned long MAX_MQTT_RECONNECT_DELAY_MS = 60000;
static const unsigned long STATUS_MIN_INTERVAL_MS = 5000;
static const unsigned long STATUS_HEARTBEAT_MS = 60000;
static unsigned long mqttReconnectDelayMs = RECONNECT_DELAY_MS;
static const unsigned long WIFI_RECONNECT_DELAY_MS = 10000;  
static const unsigned long NTP_SYNC_DELAY_MS = 30000;  // Delay between NTP Syncs (30 seconds)

// Forward Function declarations
void cryptPayload(uint8_t* data, size_t length, const uint8_t* mac, uint32_t bootNonceValue, uint16_t msgCount);
void setup();
void loop();
void drawMain();
void configureSystemTime();
bool refreshRtcTrustFromSystemClock();
void formatLocalTime(time_t utcTime, char* buffer, size_t bufferSize, const char* format);
bool mqttPublish(const char* topic, JsonDocument& doc, bool retained = false);
bool sendUpdateBeacon();
bool isClockValid(time_t currentTime);
bool hasClockDrift(time_t currentTime, time_t referenceTime, double thresholdSeconds);
void mqttConnect();
String mqttTopic(const String* mac, const char* mode);
void jsonBuild(const void* rawPayload, JsonDocument& doc, const String* mac, const char* mode);
void initRadio();

WiFiClientSecure espClient; // Use secure client for TLS
PubSubClient mqttClient(espClient);

// Check if clock is set != 1970 or CUSTOM_EPOCH + boot
bool isClockValid(time_t timeNow) {
    return isClockValidSince(timeNow, (unsigned long)uptime::getSeconds());
}

void formatLocalTime(time_t utcTime, char* buffer, size_t bufferSize, const char* format) {
    struct tm localTimeInfo;
    localtime_r(&utcTime, &localTimeInfo);
    strftime(buffer, bufferSize, format, &localTimeInfo);
}

void configureSystemTime() {
    configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov", "time.google.com");
}

bool refreshRtcTrustFromSystemClock() {
    time_t currentTime;
    time(&currentTime);
    if (isClockValid(currentTime)) {
        hasValidRtcTime = true;
    }
    return hasValidRtcTime;
}

void cryptPayload(uint8_t* data, size_t length, const uint8_t* mac, uint32_t bootNonceValue, uint16_t msgCount) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, AES_NETWORK_KEY, 128); // 128-bit AES
    
    // Match the sensor's MAC-scoped, per-boot CTR nonce.
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

bool sendUpdateBeacon() {
    uint8_t beacon = CONFIG_UPDATE_PENDING_FLAG;
    xSemaphoreTake(radioSemaphore, 0); // clear any pending semaphore
    int txState = radio.startTransmit(&beacon, 1);
    if (txState == RADIOLIB_ERR_NONE) {
        if (xSemaphoreTake(radioSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
            Logger.info(MYLOG, "::%s:: Update beacon transmitted." , string(__func__).substr(0, 5).c_str());
            return true;
        } else {
            Logger.warning(MYLOG, "::%s:: Update beacon TX timeout!", string(__func__).substr(0, 5).c_str());
            return false;
        }
    }
    Logger.error(MYLOG, "::%s:: Update beacon startTransmit failed, code %d", string(__func__).substr(0, 5).c_str(), txState);
    return false;
}

const char* getWifiStatusString(wl_status_t status) {
  switch (status) {
    case 0:      return "IDLE_STATUS";
    case 1:      return "NO_SSID_AVAIL";
    case 2:      return "SCAN_COMPLETED";
    case 3:      return "CONNECTED";
    case 4:      return "CONNECT_FAILED";
    case 5:      return "CONNECTION_LOST";
    case 6:      return "DISCONNECTED";
    default:     return "UNKNOWN_STATUS";
  }
}

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266)
ICACHE_RAM_ATTR
#elif defined(ESP32)
IRAM_ATTR
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

void setup()
{
    uptime::calculateUptime();

    setupBoards();

    // Initialize Logger with Serial and SD output
    // Register console output (Serial)
    Logger.registerSerial(MYLOG, ELOG_LEVEL_DEBUG, "GW");

    // Configure and register SD card output
    Logger.configureSd(SDCardSPI, SDCARD_CS, 2000000, SHARED_SPI);
    // SD card is already initialized by setupBoards(), just register the logger
    Logger.registerSd(MYLOG, ELOG_LEVEL_DEBUG, "gateway", ELOG_FLAG_NONE, 102400);

    // Simulate the time by providing a fixed time to the RTC (You can also use the NTP time)
    Logger.provideTime(2024, 01, 01, 0, 0, 0); //<-- CUSTOM_EPOCH
    // Initialize the FreeRTOS semaphore
    radioSemaphore = xSemaphoreCreateBinary();

    // Initialize WiFi and NTP in the background (non-blocking)
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Setup TLS client (use setInsecure for testing without CA cert, 
    // or provide CA cert using espClient.setCACert(root_ca) for production)
    espClient.setInsecure();

    // Setup MQTT
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

    
    // Attempt to recover time from ESP32's hardware RTC (survives software resets)
    time_t sysTime;
    time(&sysTime);
    refreshRtcTrustFromSystemClock();  // Check and set hasValidRtcTime based on current RTC time
    
    if (hasValidRtcTime) {
        Logger.info(MYLOG, "::%s:: Recovered time from internal ESP32 RTC after soft reset.", string(__func__).substr(0, 5).c_str());
    } else {
        configureSystemTime(); // Ensure SNTP is configured if RTC time is not valid
        time(&sysTime);
        Logger.info(MYLOG, "::%s:: RTC not valid, syncing time from NTP server.", string(__func__).substr(0, 5).c_str());
        
        // Check if NTP sync succeeded
        if (!isClockValid(sysTime)) {
            Logger.warning(MYLOG, "::%s:: NTP sync failed. setup will proceed with invalid time.", string(__func__).substr(0, 5).c_str());
        }
    }
    

    // When the power is turned on, a delay is required.
    vTaskDelay(pdMS_TO_TICKS(1500));

    initRadio();

    drawMain();
}

void initRadio() {
#ifdef  RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
#endif

    // initialize radio with default settings
    int state = radio.begin();

    if (state == RADIOLIB_ERR_NONE) {
        Logger.info(MYLOG, "::%s:: [%s]: Radio Initializing ... success!", string(__func__).substr(0, 5).c_str(), RADIO_TYPE_STR);
    } else {
        Logger.error(MYLOG, "::%s:: [%s]: Radio Initializing ... failed, code %d", string(__func__).substr(0, 5).c_str(), RADIO_TYPE_STR, state);
        while (true);
    }

    // Set interrupt callback for packet reception
    radio.setPacketReceivedAction(setFlag);

    /*
    *   Sets carrier frequency.
    *   SX1278/SX1276 : Allowed values range from 137.0 MHz to 525.0 MHz.
    *   SX1268/SX1262 : Allowed values are in range from 150.0 to 960.0 MHz.
    *   SX1280        : Allowed values are in range from 2400.0 to 2500.0 MHz.
    *   LR1121        : Allowed values are in range from 150.0 to 960.0 MHz, 1900 - 2200 MHz and 2400 - 2500 MHz. Will also perform calibrations.
    * * * */

    // Declare configErrors before use
    int configErrors = 0;

    if (radio.setFrequency(CONFIG_RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Logger.error(MYLOG, "::%s:: Selected frequency is invalid for this module!", string(__func__).substr(0, 5).c_str());
        configErrors++;
    } else {
        Logger.info(MYLOG, "::%s:: Freq: %.1f MHz OK", string(__func__).substr(0, 5).c_str(), CONFIG_RADIO_FREQ);
    }

    /*
    *   Sets LoRa link bandwidth.
    *   SX1278/SX1276 : Allowed values are 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250 and 500 kHz. Only available in %LoRa mode.
    *   SX1268/SX1262 : Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
    *   SX1280        : Allowed values are 203.125, 406.25, 812.5 and 1625.0 kHz.
    *   LR1121        : Allowed values are 62.5, 125.0, 250.0 and 500.0 kHz.
    * * * */
    if (radio.setBandwidth(CONFIG_RADIO_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Logger.error(MYLOG, "::%s:: Selected bandwidth is invalid for this module!", string(__func__).substr(0, 5).c_str());
        configErrors++;
    } else {
        Logger.info(MYLOG, "::%s:: BW: %.1f kHz OK", string(__func__).substr(0, 5).c_str(), CONFIG_RADIO_BW);
    }


    /*
    * Sets LoRa link spreading factor.
    * SX1278/SX1276 :  Allowed values range from 6 to 12. Only available in LoRa mode.
    * SX1262        :  Allowed values range from 5 to 12.
    * SX1280        :  Allowed values range from 5 to 12.
    * LR1121        :  Allowed values range from 5 to 12.
    * * * */
    if (radio.setSpreadingFactor(9) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Logger.error(MYLOG, "::%s:: Selected spreading factor is invalid for this module!", string(__func__).substr(0, 5).c_str());
        configErrors++;
    } else {
        Logger.info(MYLOG, "::%s:: SF: 9 OK", string(__func__).substr(0, 5).c_str());
    }

    /*
    * Sets LoRa coding rate denominator.
    * SX1278/SX1276/SX1268/SX1262 : Allowed values range from 5 to 8. Only available in LoRa mode.
    * SX1280        :  Allowed values range from 5 to 8.
    * LR1121        :  Allowed values range from 5 to 8.
    * * * */
    if (radio.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Logger.error(MYLOG, "::%s:: Selected coding rate is invalid for this module!", string(__func__).substr(0, 5).c_str());
        configErrors++;
    } else {
        Logger.info(MYLOG, "::%s:: CR: 4/5 OK", string(__func__).substr(0, 5).c_str());
    }

    /*
    * Sets LoRa sync word.
    * SX1278/SX1276/SX1268/SX1262/SX1280 : Sets LoRa sync word. Only available in LoRa mode.
    * * */
    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
        Logger.error(MYLOG, "::%s:: Unable to set sync word!", string(__func__).substr(0, 5).c_str());
        configErrors++;
    } else {
        Logger.info(MYLOG, "::%s:: SW: 0xAB OK", string(__func__).substr(0, 5).c_str());
    }

    /*
    * Sets transmission output power.
    * SX1278/SX1276 :  Allowed values range from -3 to 15 dBm (RFO pin) or +2 to +17 dBm (PA_BOOST pin). High power +20 dBm operation is also supported, on the PA_BOOST pin. Defaults to PA_BOOST.
    * SX1262        :  Allowed values are in range from -9 to 22 dBm. This method is virtual to allow override from the SX1261 class.
    * SX1268        :  Allowed values are in range from -9 to 22 dBm.
    * SX1280        :  Allowed values are in range from -18 to 13 dBm. PA Version range : -18 ~ 3dBm
    * LR1121        :  Allowed values are in range from -17 to 22 dBm (high-power PA) or -18 to 13 dBm (High-frequency PA), PA Version range : -9 ~ 0dBm
    * * * */
    if (radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Logger.error(MYLOG, "::%s:: Selected output power is invalid for this module!", string(__func__).substr(0, 5).c_str());
        configErrors++;
    } else {
        Logger.info(MYLOG, "::%s:: TX Power: %ddBm OK", string(__func__).substr(0, 5).c_str(), CONFIG_RADIO_OUTPUT_POWER);
    }

#if !defined(USING_SX1280) && !defined(USING_LR1121) && !defined(USING_SX1280PA)
    /*
    * Sets current limit for over current protection at transmitter amplifier.
    * SX1278/SX1276 : Allowed values range from 45 to 120 mA in 5 mA steps and 120 to 240 mA in 10 mA steps.
    * SX1262/SX1268 : Allowed values range from 45 to 120 mA in 2.5 mA steps and 120 to 240 mA in 10 mA steps.
    * NOTE: set value to 0 to disable overcurrent protection
    * * * */
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Logger.error(MYLOG, "::%s:: Selected current limit is invalid for this module!", string(__func__).substr(0, 5).c_str());
        configErrors++;
    } else {
        Logger.info(MYLOG, "::%s:: Current Limit: 140 mA OK", string(__func__).substr(0, 5).c_str());
    }
#endif

    /*
    * Sets preamble length for LoRa or FSK modem.
    * SX1278/SX1276 : Allowed values range from 6 to 65535 in %LoRa mode or 0 to 65535 in FSK mode.
    * SX1262/SX1268 : Allowed values range from 1 to 65535.
    * SX1280        : Allowed values range from 1 to 65535. preamble length is multiple of 4
    * LR1121        : Allowed values range from 1 to 65535.
    * * */
    // Removed preamble override to let it use default (8) matching the sensor code.
    /*if (radio.setPreambleLength(16) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        while (true);
    }*/

    // Enables or disables CRC check of received packets.
    if (radio.setCRC(true) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Logger.error(MYLOG, "::%s:: Selected CRC configuration is invalid for this module!", string(__func__).substr(0, 5).c_str());
        configErrors++;
    } else {
        Logger.info(MYLOG, "::%s:: CRC: Enabled OK", string(__func__).substr(0, 5).c_str());
    }

    // ============================================================================
    // VALIDATE CONFIGURATION AND REPORT STATUS
    // ============================================================================
    
    if (configErrors > 0) {
        Logger.warning(MYLOG, "::%s:: Radio configuration completed with %d error(s).", string(__func__).substr(0, 5).c_str(), configErrors);
        Logger.warning(MYLOG, "::%s:: The radio may still function, but some parameters were not set correctly.", string(__func__).substr(0, 5).c_str());
        Logger.warning(MYLOG, "::%s:: Please check hardware connections and power supply stability.", string(__func__).substr(0, 5).c_str());
    } else {
        Logger.info(MYLOG, "::%s:: All radio parameters configured successfully!", string(__func__).substr(0, 5).c_str());
    }

    // Delay to allow radio internal circuits to stabilize after configuration
    vTaskDelay(pdMS_TO_TICKS(100));

#if  defined(USING_LR1121)
#if defined(USING_LR1121PA)
    if (CONFIG_RADIO_FREQ < 2400) {
        Logger.info(MYLOG, "::%s:: LR1121 PA Version Using low frequency switch table for PA version", string(__func__).substr(0, 5).c_str());
        radio.setRfSwitchTable(pa_version_rf_switch_dio_pins, low_freq_switch_table);
    } else {
        Logger.info(MYLOG, "::%s:: LR1121 PA Version Using high frequency switch table for PA version", string(__func__).substr(0, 5).c_str());
        radio.setRfSwitchTable(pa_version_rf_switch_dio_pins, high_freq_switch_table);
    }
#else   //  Version without PA rf switch table
    Logger.info(MYLOG, "::%s:: LR1121 without PA Version", string(__func__).substr(0, 5).c_str());
    static const uint32_t rfswitch_dio_pins[] = {
        RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
        RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC
    };
    static const Module::RfSwitchMode_t rfswitch_table[] = {
        // mode                  DIO5  DIO6
        { LR11x0::MODE_STBY,   { LOW,  LOW  } },
        { LR11x0::MODE_RX,     { HIGH, LOW  } },
        { LR11x0::MODE_TX,     { LOW,  HIGH } },
        { LR11x0::MODE_TX_HP,  { LOW,  HIGH } },
        { LR11x0::MODE_TX_HF,  { LOW,  LOW  } },
        { LR11x0::MODE_GNSS,   { LOW,  LOW  } },
        { LR11x0::MODE_WIFI,   { LOW,  LOW  } },
        END_OF_MODE_TABLE,
    };
    radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
#endif /*USING_LR1121PA*/

    // LR1121 TCXO Voltage 2.85~3.15V
    radio.setTCXO(3.0);

#endif /*USING_LR1121*/

#ifdef USING_DIO2_AS_RF_SWITCH
#ifdef USING_SX1262
    // Some SX126x modules use DIO2 as RF switch. To enable
    // this feature, the following method can be used.
    // NOTE: As long as DIO2 is configured to control RF switch,
    //       it can't be used as interrupt pin!
    if (radio.setDio2AsRfSwitch() != RADIOLIB_ERR_NONE) {
        Logger.error(MYLOG, "::%s:: Failed to set DIO2 as RF switch!", string(__func__).substr(0, 5).c_str());
        while (true);
    }
#endif //USING_SX1262
#endif //USING_DIO2_AS_RF_SWITCH


#ifdef RADIO_RX_PIN
    // SX1280 PA Version
    radio.setRfSwitchPins(RADIO_RX_PIN, RADIO_TX_PIN);
#endif


#ifdef RADIO_SWITCH_PIN
    // T-MOTION
    const uint32_t pins[] = {
        RADIO_SWITCH_PIN, RADIO_SWITCH_PIN, RADIOLIB_NC,
    };
    static const Module::RfSwitchMode_t table[] = {
        {Module::MODE_IDLE,  {0,  0} },
        {Module::MODE_RX,    {1, 0} },
        {Module::MODE_TX,    {0, 1} },
        END_OF_MODE_TABLE,
    };
    radio.setRfSwitchTable(pins, table);
#endif

#ifdef RADIO_CTRL
    Logger.info(MYLOG, "::%s:: Turn on LAN, Enter Rx mode.", string(__func__).substr(0, 5).c_str());
    /*
    * 2W and BPF LoRa LAN Control ,set HIGH turn on LAN ,RX Mode
    * */
    digitalWrite(RADIO_CTRL, HIGH);
#endif /*RADIO_CTRL*/

    vTaskDelay(pdMS_TO_TICKS(1000));

    // start listening for LoRa packets
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Logger.info(MYLOG, "::%s:: Radio started listening successfully!", string(__func__).substr(0, 5).c_str());
    } else {
        Logger.error(MYLOG, "::%s:: Radio failed to start, code %d", string(__func__).substr(0, 5).c_str(), state);
    }
}

void loop()
{
    // Service the radio before any potentially blocking network maintenance.
    while (xSemaphoreTake(radioSemaphore, 0) == pdTRUE) {
        
        // process received data from the radio 
        int numBytes = radio.getPacketLength();
        byte byteArr[256];
        
        // Memory safety: Validate packet length against buffer size
        if (numBytes <= 0 || numBytes > sizeof(byteArr)) {
            Logger.warning(MYLOG, "::%s:: Invalid packet length from radio: %d bytes", string(__func__).substr(0, 5).c_str(), numBytes);
            xSemaphoreTake(radioSemaphore, 0);
            radio.startReceive();
            continue; // Skip to next semaphore check in loop
        }
        
        int state = radio.readData(byteArr, numBytes);

        flashLed();

        if (state == RADIOLIB_ERR_NONE) {
            // capture reception information
            rssi = String(radio.getRSSI()) + "dBm";
            snr = String(radio.getSNR()) + "dB";

            // decrypt the received payload
            Logger.debug(MYLOG, "::%s:: Received %d bytes from radio", string(__func__).substr(0, 5).c_str(), numBytes);
            if (numBytes == 20 || numBytes == sizeof(TelemetryPayload)) {
                TelemetryPayload rxPayload;
                // FIX: Cast to uint8_t* to treat both source and destination as raw byte arrays
                // This prevents incorrect interpretation of bytes when copying to mixed-type struct
                uint8_t* payloadPtr = (uint8_t*)&rxPayload;
                memcpy(payloadPtr, byteArr, numBytes);
                
                size_t encryptedLength = numBytes - 12;
                uint8_t* dataPtr = ((uint8_t*)&rxPayload) + 12;
                cryptPayload(dataPtr, encryptedLength, rxPayload.mac, rxPayload.bootNonce, rxPayload.msgCount);
                // TO DO: check that received paylaod has been decrytped successfuly

                // --- TIME-CRITICAL SECTION: Respond to Sensor ---
                // This block must execute as fast as possible to meet the sensor's short RX window.
                // construct sensor configuration payload

                ConfigPayload txConfig;
                txConfig.header[0] = 'C'; // harcoded
                txConfig.header[1] = 'F'; // hardcoded
                memcpy(txConfig.targetMac, rxPayload.mac, 6);
                txConfig.networkKey = NETWORK_KEY;
                txConfig.sleepInterval = 20; // Example config will be read from mqtt
                txConfig.configVersion = 1;
                txConfig.isDevMode = 1;      // Example config will be read from mqtt

                //ensure RTC is synced before transmiting timeoffset
                if (!hasValidRtcTime) {
                    if (!refreshRtcTrustFromSystemClock()){
                        Logger.warning(MYLOG, "::%s:: Unable to Sync from NTP server.", string(__func__).substr(0, 5).c_str());
                    } 
                } 
                
                // calculate time offset
                time_t currentSysTime;
                time(&currentSysTime);
                if (hasValidRtcTime && isClockValid(currentSysTime)) {
                    txConfig.timeOffset = (uint32_t)(currentSysTime - CUSTOM_EPOCH);
                    bool timeSyncPending = (txConfig.timeOffset > 0) && (rxPayload.needsTimeSync != 0);
                } else {
                    txConfig.timeOffset = 0;
                    Logger.warning(MYLOG, "::%s:: Update Beacon will be dropped this loop.", string(__func__).substr(0, 5).c_str());
                }

                // determine and transmit the update beacon 
                bool sensorNeedsConfig = (rxPayload.sleepInterval != txConfig.sleepInterval) || ((rxPayload.isDevMode != 0) != (txConfig.isDevMode != 0));
                
                bool updatePending = (sensorNeedsConfig || rxPayload.needsTimeSync) && hasValidRtcTime; // enable beacon only if RTC is ok
                if (updatePending) {
                    // Give the Sensor time to leave TX/standby and enter its beacon RX window.
                    vTaskDelay(pdMS_TO_TICKS(50));
                    if (sendUpdateBeacon()) { // check if beacon was transmitted
                        xSemaphoreTake(radioSemaphore, 0); // clear before tx
                        int txState = radio.startTransmit((uint8_t*)&txConfig, sizeof(ConfigPayload)); // transmit sensor config
                        if (txState == RADIOLIB_ERR_NONE) {
                            if (xSemaphoreTake(radioSemaphore, pdMS_TO_TICKS(5000)) == pdTRUE) {
                                Logger.info(MYLOG, "::%s:: Config Sent", string(__func__).substr(0, 5).c_str());
                                Logger.debug(MYLOG, "::%s:: Config: Sleep: %u | DevMode: %u | TimeOffset: %lu",
                                                          string(__func__).substr(0, 5).c_str(), txConfig.sleepInterval, txConfig.isDevMode, (unsigned long)txConfig.timeOffset);
                            } else {
                                Logger.error(MYLOG, "::%s:: Config TX timeout", string(__func__).substr(0, 5).c_str());
                            }
                        } else {
                            Logger.error(MYLOG, "::%s:: Config startTransmit failed, code %d", string(__func__).substr(0, 5).c_str(), txState);
                        }
                    }
                } else {
                     Logger.info(MYLOG, "::%s:: No config or time update pending.", string(__func__).substr(0, 5).c_str());
                }
                // --- END OF TIME-CRITICAL SECTION ---
                // Clear any lingering semaphore from the TX operations
                xSemaphoreTake(radioSemaphore, 0);

                // Put radio back into receive mode immediately to not miss the next packet
                radio.startReceive();

                // --- NON-CRITICAL SECTION: Logging, Display, and MQTT ---
                // Now that the LoRa transaction is complete, we can perform slower tasks.

                // Both compact operation-mode and full development-mode packets
                // contain the core telemetry fields and must be published.
                sensorHasTelemetry = (numBytes == 20 || numBytes == sizeof(TelemetryPayload));
                if (sensorHasTelemetry) {
                    Logger.debug(MYLOG, "::%s:: Sensor payload has data", string(__func__).substr(0, 5).c_str());
                    Logger.debug(MYLOG, "::%s:: Reconstructing MAC Adress", string(__func__).substr(0, 5).c_str());
                    char macStr[18];
                    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                            rxPayload.mac[0], rxPayload.mac[1], rxPayload.mac[2],
                            rxPayload.mac[3], rxPayload.mac[4], rxPayload.mac[5]);
                    sensorMac = String(macStr);
                    Logger.debug(MYLOG, "::%s:: Normalising telemetry values for debug", string(__func__).substr(0, 5).c_str());
                    sensorTemp = rxPayload.temperature / 100.0f;
                    sensorVcc = rxPayload.battVoltage / 1000.0f;
                    sensorBatt = rxPayload.battPercent;
                    msgCount = rxPayload.msgCount;
                    sensorIsDevMode = rxPayload.isDevMode;
                    sensorNeedsTimeSync = (rxPayload.needsTimeSync != 0);
                    sensorSleepInterval = rxPayload.sleepInterval;
                    if (rxPayload.isDevMode == 1){
                        sensorBattCurrent = rxPayload.battCurrent;
                        sensorBattPower = rxPayload.battPower;
                        sensorFreeRam = rxPayload.freeRam;
                        sensorCpuTemp = rxPayload.cpuTemp;
                        sensorTxPower = rxPayload.txPower;
                        sensorSNR = rxPayload.lastSNR;
                        sensorRSSI = rxPayload.lastRSSI;
                        Logger.debug(MYLOG, "::%s:: DEV Mode -> CPU Temp: %dC | RAM: %uKB | I: %dmA | Pow: %dmW | TX: %ddBm | LastCfg SNR: %ddB | LastCfg RSSI: %ddBm", 
                                  string(__func__).substr(0, 5).c_str(), sensorCpuTemp, sensorFreeRam, sensorBattCurrent, sensorBattPower, sensorTxPower, sensorSNR, sensorRSSI);
                    }        
                    Logger.info(MYLOG, "::%s:: Radio Received packet! MAC: %s | Msg: %u", string(__func__).substr(0, 5).c_str(), macStr, msgCount);
                    Logger.info(MYLOG, "::%s:: Temp: %.1fC | VCC: %.2fV | Batt: %d%% | Sleep: %lu | DevMode: %u | NeedsTimeSync: %s", 
                                  string(__func__).substr(0, 5).c_str(), sensorTemp, sensorVcc, sensorBatt, (unsigned long)sensorSleepInterval, 
                                  sensorIsDevMode, sensorNeedsTimeSync ? "yes" : "no");
                    
                    // compose telemetry json and publish to mqtt broker                    
                    if (mqttClient.connected()) {
                        StaticJsonDocument<512> docCore; 
                        jsonBuild(&rxPayload, docCore, &sensorMac, "core"); 
                        String telemetryDataTopic = mqttTopic(&sensorMac, "core");
                        mqttPublish(telemetryDataTopic.c_str(), docCore);
                        Logger.debug(MYLOG, "::%s:: Sensor telemetry data published: %s : %s", string(__func__).substr(0, 5).c_str(), telemetryDataTopic.c_str(), docCore.as<String>().c_str());

                        if (rxPayload.isDevMode == 1){
                            StaticJsonDocument<512> docDev; 
                            jsonBuild(&rxPayload, docDev, &sensorMac, "dev");
                            String devTelemetryDataTopic = mqttTopic(&sensorMac, "dev");
                            mqttPublish(devTelemetryDataTopic.c_str(), docDev);
                            Logger.debug(MYLOG, "::%s:: Sensor dev telemetry data published: %s : %s", string(__func__).substr(0, 5).c_str(), devTelemetryDataTopic.c_str(), docDev.as<String>().c_str());  
                        }
                    }
                    
                }
                screenNum = 0; // Force switch to data screen on new packet
                lastDisplayUpdate = millis();
                drawMain();

            } else {
                Logger.warning(MYLOG, "::%s:: Received unknown packet of %d bytes", string(__func__).substr(0, 5).c_str(), numBytes);
                xSemaphoreTake(radioSemaphore, 0);
                radio.startReceive();
            }

        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            // packet was received, but is malformed
            Logger.error(MYLOG, "::%s:: CRC error!", string(__func__).substr(0, 5).c_str());
            xSemaphoreTake(radioSemaphore, 0);
            radio.startReceive();
        } else {
            // some other error occurred
            Logger.error(MYLOG, "::%s:: Receive failed, code %d", string(__func__).substr(0, 5).c_str(), state);
            xSemaphoreTake(radioSemaphore, 0);
            radio.startReceive();
        }

    }

    // Connect WiFi/MQTT only after any pending downlink has been sent.
    static bool wasWifiConnected = false;
    bool isWifiConnected = WiFi.isConnected();
    if (isWifiConnected) {
        if (!wasWifiConnected) {
            Logger.info(MYLOG, "::%s:: WiFi connected. Device IP: %s", string(__func__).substr(0, 5).c_str(), WiFi.localIP().toString().c_str());
            wasWifiConnected = true;
        }
        if (!mqttClient.connected()) {
            mqttConnect();
        }
        static bool mqttWasConnected = false;
        bool mqttJustConnected = mqttClient.connected() && !mqttWasConnected;
        mqttWasConnected = mqttClient.connected();
        mqttClient.loop();
        GatewayStatus status{};
        status.gatewayRssi = (int16_t)radio.getRSSI();
        status.gatewaySnr = radio.getSNR();
        snprintf(status.wifiStatus, sizeof(status.wifiStatus), "%s", getWifiStatusString(WiFi.status()));
        snprintf(status.ip, sizeof(status.ip), "%s", WiFi.localIP().toString().c_str());
        status.freeRam = ESP.getFreeHeap();
        static GatewayStatus lastStatus{};
        static bool hasPublishedStatus = false;
        static unsigned long lastStatusPublishMs = 0;
        // Heap fluctuates continuously; include it in published status but do
        // not let it independently trigger another MQTT publication.
        GatewayStatus comparableStatus = status;
        GatewayStatus comparableLastStatus = lastStatus;
        comparableStatus.freeRam = 0;
        comparableLastStatus.freeRam = 0;
        bool statusChanged = !hasPublishedStatus || memcmp(&comparableStatus, &comparableLastStatus, sizeof(status)) != 0;
        bool statusDue = (millis() - lastStatusPublishMs) >= STATUS_HEARTBEAT_MS;
        bool changeDue = statusChanged && (millis() - lastStatusPublishMs) >= STATUS_MIN_INTERVAL_MS;
        if (mqttJustConnected || changeDue || statusDue) {
            StaticJsonDocument<256> statusDoc;
            jsonBuild(&status, statusDoc, nullptr, "gatewayStatus");
            String statusTopic = mqttTopic(nullptr, "gatewayStatus");
            if (mqttPublish(statusTopic.c_str(), statusDoc, true)) {
                lastStatus = status;
                hasPublishedStatus = true;
                lastStatusPublishMs = millis();
            }
        }
    } else {
        if (wasWifiConnected) {
            Logger.warning(MYLOG, "::%s:: WiFi %s. Reconnecting..", string(__func__).substr(0, 5).c_str(), getWifiStatusString(WiFi.status()));
            wasWifiConnected = false;
        }
        if (millis() - lastWifiReconnectAttempt > WIFI_RECONNECT_DELAY_MS) {
            Logger.warning(MYLOG, "::%s:: WiFi %s. Reconnecting...", string(__func__).substr(0, 5).c_str(), getWifiStatusString(WiFi.status()));
            WiFi.reconnect();
            lastWifiReconnectAttempt = millis();
        }
    }

    static unsigned long lastNtpCheck = 0;
    if (millis() - lastNtpCheck > NTP_SYNC_DELAY_MS) {
        time_t now;
        time(&now);
        if (WiFi.isConnected() && !isClockValid(now)) {
            Logger.warning(MYLOG, "::%s:: Time invalid, NTP syncing...", string(__func__).substr(0, 5).c_str());
            configureSystemTime();
        }
        lastNtpCheck = millis();
    }

    // Rotate screen every 5 seconds
    if (millis() - lastDisplayUpdate > 5000) {
        screenNum = (screenNum + 1) % 2;
        lastDisplayUpdate = millis();
        drawMain();
    }
}

String mqttTopic(const String* mac, const char* mode) {
    if (!mode) return String(MQTT_TOPIC_PREFIX) + "/error";
    
    switch(mode[0]) { 
    case 'c': { // "core" starts with 'c'
        if (!mac) return String(MQTT_TOPIC_PREFIX) + "/error";
        return String(MQTT_TOPIC_PREFIX) + "/sensor/" + mac->c_str() + "/data";
    }
    case 'd': { // "dev" starts with 'd'  
        if (!mac) return String(MQTT_TOPIC_PREFIX) + "/error";
        return String(MQTT_TOPIC_PREFIX) + "/sensor/" + mac->c_str() + "/telemetry";
    }
    case 'g': { // "gatewayStatus" starts with 'g'
        return String(MQTT_TOPIC_PREFIX) + "/gateway/status";
    }
    // disabled config topic will be read for updates
    // case 'p': { // "payloadConfig" starts with 'p' (since 'c' is taken by core)
    //     return String(MQTT_TOPIC_PREFIX) + "/config/" + mac->c_str();
    // }
    default:
        return String(MQTT_TOPIC_PREFIX) + "/error";
    }    
}

void jsonBuild(const void* rawPayload, JsonDocument& doc, const String* mac, const char* mode) {
    doc.clear(); 
    
    // Always add hardware timestamp. If NTP fails, this makes the failure visible (e.g., 1970 dates)
    static unsigned long lastTimestamp = 0;  // Track when timestamp was last updated
    static char cachedTimeStr[24];
    
    // Cache timestamp for 1 second - fixed: compare elapsed time, not against constant
    if (lastTimestamp == 0 || (millis() - lastTimestamp) > 1000) {
        time_t sysTime = time(nullptr);
        formatLocalTime(sysTime, cachedTimeStr, sizeof(cachedTimeStr), "%Y-%m-%dT%H:%M:%S");
        lastTimestamp = millis();  // Record when we updated the timestamp
        if (lastTimestamp == 0) lastTimestamp = 1; // Edge case: prevent re-triggering if millis() is exactly 0
    }
    doc["timestamp"] = cachedTimeStr;
    
    // Re-define JSON macros locally since they are #undef'd at the end of LoRaHomeCommon.h
    #define TO_JSON_FIELD(type, name, json_key) \
        doc[json_key] = payload.name;

    #define TO_JSON_ARRAY(type, name, size, json_key) \
        { \
            JsonArray arr = doc[json_key].to<JsonArray>(); \
            for(int i = 0; i < size; i++) { arr.add(payload.name[i]); } \
        }

    #define TO_JSON_STRING(name, max_len, json_key) \
        { \
            char temp[max_len + 1]; \
            memcpy(temp, payload.name, max_len); \
            temp[max_len] = '\0'; \
            doc[json_key] = temp; \
        }

    // Safeguard against null pointers
    if (rawPayload != nullptr && mode != nullptr) {
        // Conditionally create json - check first character (fastest possible check)
        switch(mode[0]) { 
            case 'c': { // "core" starts with 'c'
                const TelemetryPayload& payload = *static_cast<const TelemetryPayload*>(rawPayload);
                TELEMETRY_CORE_FIELDS(TO_JSON_FIELD, TO_JSON_ARRAY, TO_JSON_STRING);
                break;
            }
            case 'd': { // "dev" starts with 'd'  
                const TelemetryPayload& payload = *static_cast<const TelemetryPayload*>(rawPayload);
                TELEMETRY_DEV_FIELDS(TO_JSON_FIELD, TO_JSON_ARRAY, TO_JSON_STRING);
                break;
            }
            case 'g': { // "gatewayStatus" starts with 'g'
                const GatewayStatus& payload = *static_cast<const GatewayStatus*>(rawPayload);
                GATEWAY_STATUS(TO_JSON_FIELD, TO_JSON_ARRAY, TO_JSON_STRING);
                break;
            }
            case 'p': { // "payloadConfig" starts with 'p' (since 'c' is taken by core)
                const ConfigPayload& payload = *static_cast<const ConfigPayload*>(rawPayload);
                CONFIG_PAYLOAD(TO_JSON_FIELD, TO_JSON_ARRAY, TO_JSON_STRING);
                break;
            }
            default:
                break;
        }
    } else{
        Logger.error(MYLOG, "::%s:: null pointer exception caused by rawPayload or mode", string(__func__).substr(0, 5).c_str());
    }

    #undef TO_JSON_FIELD
    #undef TO_JSON_ARRAY
    #undef TO_JSON_STRING

    // Override mac array with formatted string if provided
    if (mac) {
        doc["mac"] = *mac;
    }
}
  
bool mqttPublish(const char* topic, JsonDocument& doc, bool retained) {
    if (!mqttClient.connected()) {
        Logger.warning(MYLOG, "::%s:: MQTT not connected. Cannot publish to %s", string(__func__).substr(0, 5).c_str(), topic);
        return false;
    }
    char jsonBuffer[256];
    size_t n = serializeJson(doc, jsonBuffer);
    if (n > 0) {
        if (mqttClient.publish(topic, jsonBuffer, retained)) {
            Logger.info(MYLOG, "::%s:: MQTT published to topic: %s", string(__func__).substr(0, 5).c_str(), topic);
            Logger.debug(MYLOG, "::%s:: Payload: %s", string(__func__).substr(0, 5).c_str(), jsonBuffer);
            return true;
        } else {
            Logger.error(MYLOG, "::%s:: MQTT publish failed to topic: %s", string(__func__).substr(0, 5).c_str(), topic);
        }
    } else {
        Logger.error(MYLOG, "::%s:: JSON serialization failed for topic: %s", string(__func__).substr(0, 5).c_str(), topic);
    }
    return false;
}

void mqttConnect() {
    if (millis() - lastMqttReconnectAttempt > mqttReconnectDelayMs) {
        Logger.info(MYLOG, "::%s:: Attempting MQTT connection...", string(__func__).substr(0, 5).c_str());
        String clientId = "LoraHomeGW-" + String((uint32_t)ESP.getEfuseMac(), HEX);
        
        if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
            Logger.info(MYLOG, "::%s:: MQTT connected!", string(__func__).substr(0, 5).c_str());
            mqttConnected = true;
            mqttReconnectDelayMs = RECONNECT_DELAY_MS;
            GatewayStatus status{};
            status.gatewayRssi = (int16_t)radio.getRSSI();
            status.gatewaySnr = radio.getSNR();
            snprintf(status.wifiStatus, sizeof(status.wifiStatus), "%s", getWifiStatusString(WiFi.status()));
            snprintf(status.ip, sizeof(status.ip), "%s", WiFi.localIP().toString().c_str());
            status.freeRam = ESP.getFreeHeap();
            StaticJsonDocument<256> statusDoc;
            jsonBuild(&status, statusDoc, nullptr, "gatewayStatus");
            String statusTopic = mqttTopic(nullptr, "gatewayStatus");
            mqttPublish(statusTopic.c_str(), statusDoc);
        } else {
            Logger.warning(MYLOG, "::%s:: MQTT connect failed, rc=%d. Trying again later.", string(__func__).substr(0, 5).c_str(), mqttClient.state());
            mqttConnected = false;
            mqttReconnectDelayMs = min(mqttReconnectDelayMs * 2, MAX_MQTT_RECONNECT_DELAY_MS);
        }
        lastMqttReconnectAttempt = millis();
    }
}

void drawMain()
{
    if (disp) {
        disp->clearBuffer();
        disp->drawRFrame(0, 0, 128, 64, 5);
        disp->setFont(u8g2_font_pxplusibmvga8_mr);
        
        if (screenNum == 0) {
            disp->setCursor(5, 15);
            disp->print("MAC:");
            disp->setCursor(35, 15);
            disp->print(sensorMac);
            
            disp->setCursor(5, 30);
            disp->printf("Temp: %.1f C", sensorTemp);
            
            disp->setCursor(5, 45);
            disp->printf("Bat: %.2fV (%d%%)", sensorVcc, sensorBatt);
            
            disp->setCursor(5, 60);
            disp->printf("Msg Cnt: %u", msgCount);
            
        } else {
            disp->setCursor(5, 15);
            disp->print("-- GW Status --");
            
            disp->setCursor(5, 30);
            if (WiFi.isConnected()) {
                disp->print(WiFi.localIP().toString());
            } else {
                disp->print("WiFi: NC");
            }
            
            disp->setCursor(5, 45);
            time_t sysTime;
            time(&sysTime);
            if (isClockValid(sysTime)) {
                struct tm timeinfo;
                localtime_r(&sysTime, &timeinfo);
                char timeStr[20];
                strftime(timeStr, sizeof(timeStr), "%y-%m-%d %H:%M", &timeinfo);
                disp->print(timeStr);
            } else {
                disp->print("Time: Syncing...");
            }
            
            disp->setCursor(5, 60);
            disp->printf("S:%s R:%s", snr.c_str(), rssi.c_str());
        }
        
        disp->sendBuffer();
    }
}
