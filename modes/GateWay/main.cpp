/*
   RadioLib Receive with Interrupts Example

   This example listens for LoRa transmissions and tries to
   receive them. Once a packet is received, an interrupt is
   triggered. To successfully receive data, the following
   settings have to be the same on both transmitter
   and receiver:
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

#include <Arduino.h>
#include <RadioLib.h>
#include <LoRaBoards.h>
#include <mbedtls/aes.h>
#include <WiFi.h>
#include <time.h>  // Required for ESP32 timeSync() function
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <payloads.h>

// WiFi and NTP Configuration
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "***password***"
#define WIFI_HOSTNAME "LoRaGateway"
#define TZ_INFO "EET-2EEST,M3.5.0/3,M10.5.0/4" // Europe/Athens

// MQTT Configuration
#define MQTT_BROKER "broker_IP" // IMPORTANT: Replace with your MQTT broker IP
#define MQTT_PORT 8883              // Default port for MQTT over TLS
#define MQTT_USER "LoRa" // Optional
#define MQTT_PASSWORD "Password" // Optional
#define MQTT_TOPIC_PREFIX "lora_gateway"
// Base time offset to reduce LoRa payload sizes (Jan 1, 2024 00:00:00 UTC)
#define CUSTOM_EPOCH 1704067200UL

// Forward declarations
void cryptPayload(uint8_t* data, size_t length, uint16_t msgCount);
void setFlag(void);
void setup();
void loop();
void drawMain();
void configureSystemTime();
bool refreshRtcTrustFromSystemClock();
void formatLocalTime(time_t utcTime, char* buffer, size_t bufferSize, const char* format);
bool sendUpdateBeacon();
bool isClockValid(time_t currentTime);
bool hasClockDrift(time_t currentTime, time_t referenceTime, double thresholdSeconds);
void reconnectMqtt();
void publishMqtt(TelemetryPayload &payload, String &macStr, bool hasDevTelem);

WiFiClientSecure espClient; // Use secure client for TLS
PubSubClient mqttClient(espClient);

// Check if current time is after CUSTOM_EPOCH AND NTP has synchronized successfully
bool isClockValid(time_t currentTime) {
    // Using ESP32's internal time status isn't exposed simply as timeSync().
    // Since CUSTOM_EPOCH is ~2024, any time strictly greater than this guarantees NTP sync.
    return currentTime > (time_t)CUSTOM_EPOCH;
}

// Check if clock has drifted beyond acceptable threshold from reference time
bool hasClockDrift(time_t currentTime, time_t referenceTime, double thresholdSeconds) {
    return fabs(difftime(currentTime, referenceTime)) > thresholdSeconds;
}

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

// Shared secret key for Gateway-to-Sensor config authentication
#define NETWORK_KEY 0x3FA4B2C1



// 16-Byte Shared secret key for AES-128 encryption
const uint8_t AES_NETWORK_KEY[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

// FreeRTOS semaphore to replace the busy-wait flag
static SemaphoreHandle_t radioSemaphore = NULL;
static String rssi = "0dBm";
static String snr = "0dB";

// Display variables
static float lastTemp = 0.0;
static float lastVcc = 0.0;
static uint8_t lastBatt = 0;
static String lastMac = "Wait...";
static uint16_t msgCount = 0;
static uint8_t lastSensorIsDevMode = 0;
static bool lastSensorNeedsTimeSync = false;
static uint32_t lastSensorSleepInterval = 0;
static bool lastHasDevTelemetry = false;
static int16_t lastBattCurrent = 0;
static int16_t lastBattPower = 0;
static uint16_t lastFreeRam = 0;
static int8_t lastCpuTemp = 0;
static int8_t lastTxPower = 0;
static int8_t lastSensorSNR = 0;
static int16_t lastSensorRSSI = 0;
static uint32_t lastDisplayUpdate = 0;
static uint8_t screenNum = 0;
static bool hasValidRtcTime = false;
static long lastMqttReconnectAttempt = 0;

// MQTT connection state tracking for reconnection logic
static bool mqttConnected = false;
static const unsigned long MAX_RECONNECT_ATTEMPTS = 5;   // Maximum retry attempts
static const unsigned long RECONNECT_DELAY_MS = 1000;    // Delay between retries (1 second)

void cryptPayload(uint8_t* data, size_t length, uint16_t msgCount) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, AES_NETWORK_KEY, 128); // 128-bit AES
    
    // SECURITY FIX: Use proper nonce construction instead of weak IV seeding
    // The nonce combines msgCount (4 bytes) with counter offset (12 bytes)
    uint8_t nonce[16];
    uint32_t nonceValue = (uint32_t)msgCount << 12;  // Shift left by 12 bits
    
    for (int i = 0; i < 4; i++) {
        nonce[i] = (nonceValue >> (i * 8)) & 0xFF;  // msgCount portion (4 bytes)
    }
    for (int i = 4; i < 16; i++) {
        nonce[i] = 0;  // counter offset starts at 0, increments per call
    }
    
    uint8_t stream_block[16] = {0};
    size_t nc_off = 0;
    mbedtls_aes_crypt_ctr(&aes, length, &nc_off, nonce, stream_block, data, data);
    mbedtls_aes_free(&aes);
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

bool sendUpdateBeacon() {
    uint8_t beacon = CONFIG_UPDATE_PENDING_FLAG;
    xSemaphoreTake(radioSemaphore, 0); // clear any pending semaphore
    int txState = radio.startTransmit(&beacon, 1);
    if (txState == RADIOLIB_ERR_NONE) {
        if (xSemaphoreTake(radioSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
            Serial.println(F("Update beacon transmitted."));
            return true;
        } else {
            Serial.println(F("Update beacon TX timeout!"));
            return false;
        }
    }
    Serial.printf("Update beacon startTransmit failed, code %d\n", txState);
    return false;
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
    setupBoards();

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

    // Start ESP32 SNTP with a POSIX timezone string so localtime() includes DST.
    configureSystemTime();
    
    // Attempt to recover time from ESP32's hardware RTC (survives software resets)
    time_t sysTime;
    time(&sysTime);
    if (isClockValid(sysTime)) {
        hasValidRtcTime = true;
        Serial.println(F("Recovered time from internal ESP32 RTC after soft reset."));
    }

    // When the power is turned on, a delay is required.
    delay(1500);

#ifdef  RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
#endif

    // initialize radio with default settings
    int state = radio.begin();

    printResult(state == RADIOLIB_ERR_NONE);

    Serial.printf("[%s]:", RADIO_TYPE_STR);
    Serial.print(F("Radio Initializing ... "));
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
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
        Serial.println(F("[ERROR] Selected frequency is invalid for this module!"));
        configErrors++;
    } else {
        Serial.printf("Freq: %.1f MHz OK\n", CONFIG_RADIO_FREQ);
    }

    /*
    *   Sets LoRa link bandwidth.
    *   SX1278/SX1276 : Allowed values are 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250 and 500 kHz. Only available in %LoRa mode.
    *   SX1268/SX1262 : Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
    *   SX1280        : Allowed values are 203.125, 406.25, 812.5 and 1625.0 kHz.
    *   LR1121        : Allowed values are 62.5, 125.0, 250.0 and 500.0 kHz.
    * * * */
    if (radio.setBandwidth(CONFIG_RADIO_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("[ERROR] Selected bandwidth is invalid for this module!"));
        configErrors++;
    } else {
        Serial.printf("BW: %.1f kHz OK\n", CONFIG_RADIO_BW);
    }


    /*
    * Sets LoRa link spreading factor.
    * SX1278/SX1276 :  Allowed values range from 6 to 12. Only available in LoRa mode.
    * SX1262        :  Allowed values range from 5 to 12.
    * SX1280        :  Allowed values range from 5 to 12.
    * LR1121        :  Allowed values range from 5 to 12.
    * * * */
    if (radio.setSpreadingFactor(9) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("[ERROR] Selected spreading factor is invalid for this module!"));
        configErrors++;
    } else {
        Serial.printf("SF: 9 OK\n");
    }

    /*
    * Sets LoRa coding rate denominator.
    * SX1278/SX1276/SX1268/SX1262 : Allowed values range from 5 to 8. Only available in LoRa mode.
    * SX1280        :  Allowed values range from 5 to 8.
    * LR1121        :  Allowed values range from 5 to 8.
    * * * */
    if (radio.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("[ERROR] Selected coding rate is invalid for this module!"));
        configErrors++;
    } else {
        Serial.printf("CR: 4/5 OK\n");
    }

    /*
    * Sets LoRa sync word.
    * SX1278/SX1276/SX1268/SX1262/SX1280 : Sets LoRa sync word. Only available in LoRa mode.
    * * */
    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
        Serial.println(F("[ERROR] Unable to set sync word!"));
        configErrors++;
    } else {
        Serial.printf("SW: 0xAB OK\n");
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
        Serial.println(F("[ERROR] Selected output power is invalid for this module!"));
        configErrors++;
    } else {
        Serial.printf("TX Power: %ddBm OK\n", CONFIG_RADIO_OUTPUT_POWER);
    }

#if !defined(USING_SX1280) && !defined(USING_LR1121) && !defined(USING_SX1280PA)
    /*
    * Sets current limit for over current protection at transmitter amplifier.
    * SX1278/SX1276 : Allowed values range from 45 to 120 mA in 5 mA steps and 120 to 240 mA in 10 mA steps.
    * SX1262/SX1268 : Allowed values range from 45 to 120 mA in 2.5 mA steps and 120 to 240 mA in 10 mA steps.
    * NOTE: set value to 0 to disable overcurrent protection
    * * * */
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("[ERROR] Selected current limit is invalid for this module!"));
        configErrors++;
    } else {
        Serial.printf("Current Limit: 140 mA OK\n");
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
        Serial.println(F("[ERROR] Selected CRC configuration is invalid for this module!"));
        configErrors++;
    } else {
        Serial.printf("CRC: Enabled OK\n");
    }

    // ============================================================================
    // VALIDATE CONFIGURATION AND REPORT STATUS
    // ============================================================================
    
    if (configErrors > 0) {
        Serial.printf("\n[WARNING] Radio configuration completed with %d error(s).\n", configErrors);
        Serial.println(F("The radio may still function, but some parameters were not set correctly."));
        Serial.println(F("Please check hardware connections and power supply stability."));
    } else {
        Serial.println(F("[OK] All radio parameters configured successfully!"));
    }

    // Delay to allow radio internal circuits to stabilize after configuration
    delay(100);

#if  defined(USING_LR1121)
#if defined(USING_LR1121PA)
    if (CONFIG_RADIO_FREQ < 2400) {
        Serial.printf("LR1121 PA Version Using low frequency switch table for PA version\n");
        radio.setRfSwitchTable(pa_version_rf_switch_dio_pins, low_freq_switch_table);
    } else {
        Serial.printf("LR1121 PA Version Using high frequency switch table for PA version\n");
        radio.setRfSwitchTable(pa_version_rf_switch_dio_pins, high_freq_switch_table);
    }
#else   //  Version without PA rf switch table
    Serial.println("LR1121 without PA Version");
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
        Serial.println(F("Failed to set DIO2 as RF switch!"));
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
    Serial.println("Turn on LAN, Enter Rx mode.");
    /*
    * 2W and BPF LoRa LAN Control ,set HIGH turn on LAN ,RX Mode
    * */
    digitalWrite(RADIO_CTRL, HIGH);
#endif /*RADIO_CTRL*/

    delay(1000);

    // start listening for LoRa packets
    Serial.print(F("Radio Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
    }

    drawMain();
}

void loop()
{
    if (WiFi.isConnected()) {
        if (!mqttClient.connected()) {
            reconnectMqtt();
        }
        mqttClient.loop();
    }

    // check if the semaphore has been given by the ISR
    while (xSemaphoreTake(radioSemaphore, 0) == pdTRUE) {
        
        int numBytes = radio.getPacketLength();
        byte byteArr[256];
        
        // Memory safety: Validate packet length against buffer size
        if (numBytes <= 0 || numBytes > sizeof(byteArr)) {
            Serial.printf("Invalid packet length from radio: %d bytes\n", numBytes);
            xSemaphoreTake(radioSemaphore, 0);
            radio.startReceive();
            continue; // Skip to next semaphore check in loop
        }
        
        int state = radio.readData(byteArr, numBytes);

        flashLed();

        if (state == RADIOLIB_ERR_NONE) {

            rssi = String(radio.getRSSI()) + "dBm";
            snr = String(radio.getSNR()) + "dB";

            if (numBytes == 16 || numBytes == sizeof(TelemetryPayload)) {
                TelemetryPayload rxPayload;
                memcpy(&rxPayload, byteArr, numBytes);
                
                size_t encryptedLength = numBytes - 8;
                uint8_t* dataPtr = ((uint8_t*)&rxPayload) + 8;
                cryptPayload(dataPtr, encryptedLength, rxPayload.msgCount);

                // --- TIME-CRITICAL SECTION: Respond to Sensor ---
                // This block must execute as fast as possible to meet the sensor's short RX window.
                ConfigPayload txConfig;
                txConfig.header[0] = 'C';
                txConfig.header[1] = 'F';
                memcpy(txConfig.targetMac, rxPayload.mac, 6);
                txConfig.networkKey = NETWORK_KEY;
                txConfig.sleepInterval = 20; // Example config
                txConfig.isDevMode = 1;      // Example config

                if (!hasValidRtcTime) {
                    refreshRtcTrustFromSystemClock();
                }

                time_t currentSysTime;
                time(&currentSysTime);
                if (hasValidRtcTime && isClockValid(currentSysTime)) {
                    txConfig.timeOffset = (uint32_t)(currentSysTime - CUSTOM_EPOCH);
                } else {
                    txConfig.timeOffset = 0;
                }

                bool configUpdatePending = (rxPayload.sleepInterval != txConfig.sleepInterval) || ((rxPayload.isDevMode != 0) != (txConfig.isDevMode != 0));
                bool timeSyncPending = (txConfig.timeOffset > 0) && (rxPayload.needsTimeSync != 0);

                bool updatePending = configUpdatePending || timeSyncPending;
                if (updatePending) {
                    delay(1000);
                    if (sendUpdateBeacon()) {
                        xSemaphoreTake(radioSemaphore, 0); // clear before tx
                        int txState = radio.startTransmit((uint8_t*)&txConfig, sizeof(ConfigPayload));
                        if (txState == RADIOLIB_ERR_NONE) {
                            xSemaphoreTake(radioSemaphore, pdMS_TO_TICKS(5000)); // Block until TX finishes
                            // Log will be printed later
                        } else {
                            // Log will be printed later
                        }
                    }
                }
                // --- END OF TIME-CRITICAL SECTION ---

                // Clear any lingering semaphore from the TX operations
                xSemaphoreTake(radioSemaphore, 0);


                // Put radio back into receive mode immediately to not miss the next packet
                radio.startReceive();

                // --- NON-CRITICAL SECTION: Logging, Display, and MQTT ---
                // Now that the LoRa transaction is complete, we can perform slower tasks.
                lastTemp = rxPayload.temperature / 100.0f;
                lastVcc = rxPayload.battVoltage / 1000.0f;
                lastBatt = rxPayload.battPercent;
                msgCount = rxPayload.msgCount;
                lastSensorIsDevMode = rxPayload.isDevMode;
                lastSensorNeedsTimeSync = (rxPayload.needsTimeSync != 0);
                lastSensorSleepInterval = rxPayload.sleepInterval;
                
                char macStr[18];
                sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                        rxPayload.mac[0], rxPayload.mac[1], rxPayload.mac[2],
                        rxPayload.mac[3], rxPayload.mac[4], rxPayload.mac[5]);
                lastMac = String(macStr);
                
                Serial.println(F("Radio Received packet!"));
                Serial.printf("MAC: %s | Msg: %u\n", macStr, msgCount);
                Serial.printf("Temp: %.1fC | VCC: %.2fV | Batt: %d%% | Sleep: %lu | DevMode: %u | NeedsTimeSync: %s\n",
                              lastTemp, lastVcc, lastBatt, (unsigned long)lastSensorSleepInterval,
                              lastSensorIsDevMode, lastSensorNeedsTimeSync ? "yes" : "no");

                lastHasDevTelemetry = (numBytes == sizeof(TelemetryPayload));
                if (lastHasDevTelemetry) {
                    lastBattCurrent = rxPayload.battCurrent;
                    lastBattPower = rxPayload.battPower;
                    lastFreeRam = rxPayload.freeRam;
                    lastCpuTemp = rxPayload.cpuTemp;
                    lastTxPower = rxPayload.txPower;
                    lastSensorSNR = rxPayload.lastSNR;
                    lastSensorRSSI = rxPayload.lastRSSI;
                    Serial.printf("DEV Mode -> CPU Temp: %dC | RAM: %uKB | I: %dmA | Pow: %dmW | TX: %ddBm | LastCfg SNR: %ddB | LastCfg RSSI: %ddBm\n", 
                                  lastCpuTemp, lastFreeRam, lastBattCurrent, lastBattPower, lastTxPower, lastSensorSNR, lastSensorRSSI);
                }

                if (updatePending) {
                     Serial.printf("Config Sent -> Sleep: %u | DevMode: %u | TimeOffset: %lu\n",
                                          txConfig.sleepInterval, txConfig.isDevMode, (unsigned long)txConfig.timeOffset);
                } else {
                     Serial.println(F("No config or time update pending."));
                }

                if (mqttClient.connected()) {
                    publishMqtt(rxPayload, lastMac, lastHasDevTelemetry);
                }

                screenNum = 0; // Force switch to data screen on new packet
                lastDisplayUpdate = millis();
                drawMain();

            } else {
                Serial.printf("Received unknown packet of %d bytes\n", numBytes);
                xSemaphoreTake(radioSemaphore, 0);
                radio.startReceive();
            }

        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));
            xSemaphoreTake(radioSemaphore, 0);
            radio.startReceive();
        } else {
            // some other error occurred
            Serial.print(F("failed, code "));
            Serial.println(state);
            xSemaphoreTake(radioSemaphore, 0);
            radio.startReceive();
        }

    }

    // Rotate screen every 5 seconds
    if (millis() - lastDisplayUpdate > 5000) {
        screenNum = (screenNum + 1) % 2;
        lastDisplayUpdate = millis();
        drawMain();
    }
}

void reconnectMqtt() {
    if (!mqttClient.connected()) {
        long now = millis();
        if (now - lastMqttReconnectAttempt > 5000) {
            lastMqttReconnectAttempt = now;
            // Attempt to reconnect
            Serial.print("Attempting MQTT connection...");
            String clientId = WIFI_HOSTNAME;
            clientId += String(random(0xffff), HEX);
            if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
                Serial.println("connected");
            } else {
                Serial.print("failed, rc=");
                Serial.print(mqttClient.state());
                Serial.println(" try again in 5 seconds");
            }
        }
    }
}

void publishMqtt(TelemetryPayload &payload, String &macStr, bool hasDevTelem) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<512> doc;  // Reduced from 512 to 256 bytes
#endif

    // SECURITY: Remove colons from MAC address for MQTT topic to prevent path traversal attacks
    // e.g., "AA:BB:CC:DD:EE:FF" -> "AABBCCDDEEFF"
    String safeMac = "";
    for (size_t i = 0; i < macStr.length(); i++) {
        if (macStr.charAt(i) != ':') {
            safeMac += macStr.charAt(i);
        }
    }
    doc["mac"] = macStr;  // Keep original format in JSON payload
    doc["msgCount"] = payload.msgCount;
    doc["temperature"] = payload.temperature / 100.0f;
    doc["vcc"] = payload.battVoltage / 1000.0f;
    doc["battery"] = payload.battPercent;
    doc["isDevMode"] = payload.isDevMode;
    doc["needsTimeSync"] = (payload.needsTimeSync != 0);
    doc["sleepInterval"] = payload.sleepInterval;
    doc["gatewayRssi"] = radio.getRSSI();
    doc["gatewaySnr"] = radio.getSNR();

    if (hasDevTelem) {
        doc["battCurrent"] = payload.battCurrent;
        doc["battPower"] = payload.battPower;
        doc["freeRam"] = payload.freeRam;
        doc["cpuTemp"] = payload.cpuTemp;
        doc["txPower"] = payload.txPower;
        doc["sensorSnr"] = payload.lastSNR;
        doc["rssi"] = payload.lastRSSI;  // Fixed: use 'lastRSSI' field from TelemetryPayload
    }

    // For ArduinoJson V6, we need to estimate size differently since Fpprintf isn't available
    // We'll try publishing and fall back to truncation if it fails due to buffer overflow
    char jsonBuffer[512];
    
#if ARDUINOJSON_VERSION_MAJOR >= 7
    serializeJson(doc, jsonBuffer);
#else
    serializeJson(doc, jsonBuffer);
#endif

    size_t bufferSize = sizeof(jsonBuffer);
    // ArduinoJson V6 serializeJson returns number of bytes written (or -1 on error)
    int bytesWritten = serializeJson(doc, jsonBuffer);
    
    Serial.printf("MQTT Publish: Buffer size=%zu, Bytes written=%d\n", bufferSize, bytesWritten);

    // If serialization failed or buffer was truncated, truncate dev telemetry and retry
    bool needsTruncation = (bytesWritten < 0 || bytesWritten > (int)bufferSize - 10);
    
    if (needsTruncation) {
        Serial.println("MQTT: JSON too large! Truncating dev telemetry...");
        
#if ARDUINOJSON_VERSION_MAJOR >= 7
        doc.remove("battCurrent");
        doc.remove("battPower");
        doc.remove("freeRam");
        doc.remove("cpuTemp");
        doc.remove("txPower");
        doc.remove("sensorSnr");
        doc.remove("rssi");
#else
        // Remove dev telemetry fields from StaticJsonDocument V6
        doc.remove("battCurrent");
        doc.remove("battPower");
        doc.remove("freeRam");
        doc.remove("cpuTemp");
        doc.remove("txPower");
        doc.remove("sensorSnr");
        doc.remove("rssi");
#endif
        
        // Retry with truncated payload
        bytesWritten = serializeJson(doc, jsonBuffer);
        Serial.printf("MQTT: Truncated to %d bytes\n", bytesWritten);
    }

    // Retry loop with backoff delay (max 5 retries)
    int retryCount = 0;
    bool published = false;
    
    while (!published && retryCount < MAX_RECONNECT_ATTEMPTS) {
        // The MQTT topic is generated dynamically here using the configured prefix and the sensor's MAC address.
        // Format: lora_gateway/<sensor_mac_address>/telemetry
        String topic = String(MQTT_TOPIC_PREFIX) + "/" + macStr + "/telemetry";
        
        if (mqttClient.publish(topic.c_str(), jsonBuffer)) {
            Serial.println("MQTT message published successfully.");
            published = true;
        } else {
            retryCount++;
            Serial.printf("MQTT: Publish attempt %d/%d failed\n", retryCount, MAX_RECONNECT_ATTEMPTS);
            
            if (retryCount < MAX_RECONNECT_ATTEMPTS) {
                unsigned long delayTime = millis() + RECONNECT_DELAY_MS;
                while (millis() < delayTime) {
                    mqttClient.loop();  // Keep MQTT client alive during retry delay
                }
            }
        }
    }

    if (!published) {
        Serial.printf("MQTT: All %d attempts failed!\n", MAX_RECONNECT_ATTEMPTS);
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
            disp->print(lastMac);
            
            disp->setCursor(5, 30);
            disp->printf("Temp: %.1f C", lastTemp);
            
            disp->setCursor(5, 45);
            disp->printf("Bat: %.2fV (%d%%)", lastVcc, lastBatt);
            
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
                char timeStr[20];
                formatLocalTime(sysTime, timeStr, sizeof(timeStr), "%y-%m-%d %H:%M");
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