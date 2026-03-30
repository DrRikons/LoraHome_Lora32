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
#include "LoRaBoards.h"
#include <mbedtls/aes.h>
#include <WiFi.h>
#include <time.h>

// WiFi and NTP Configuration
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "***password***"
#define WIFI_HOSTNAME "LoRaGateway"
#define NTP_SERVER "pool.ntp.org"

// Forward declarations
void cryptPayload(uint8_t* data, size_t length, uint16_t msgCount);
void setFlag(void);
void setup();
void loop();
void drawMain();

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

// Base time offset to reduce LoRa payload sizes (Jan 1, 2024 00:00:00 UTC)
#define CUSTOM_EPOCH 1704067200UL

// 16-Byte Shared secret key for AES-128 encryption
const uint8_t AES_NETWORK_KEY[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

// Packed binary structure for highly efficient LoRa transmission (up to 26 bytes)
struct __attribute__((packed)) TelemetryPayload {
  // --- Core variables (15 bytes, always sent) ---
  uint8_t  mac[6];       // 6 bytes: Raw MAC address
  uint16_t msgCount;     // 2 bytes: Message counter (cycles at 65535)
  int16_t  temperature;  // 2 bytes: External Temp (x 100)
  uint16_t battVoltage;  // 2 bytes: Battery Voltage in mV (x 1000)
  uint8_t  battPercent;  // 1 byte:  Battery capacity (0-100%)
  uint8_t  configVer;    // 1 byte:  Config version
  uint8_t  opMode;       // 1 byte:  0=Op, 1=Dev
  
  // --- Dev mode variables (11 bytes, conditionally sent) ---
  int16_t  battCurrent;  // 2 bytes: Battery Current in mA
  int16_t  battPower;    // 2 bytes: Battery Power in mW
  uint16_t freeRam;      // 2 bytes: Free RAM in KB
  int8_t   cpuTemp;      // 1 byte:  CPU Temp in C
  int8_t   txPower;      // 1 byte:  TX Power in dBm
  int8_t   lastSNR;      // 1 byte:  Last received SNR
  int16_t  lastRSSI;     // 2 bytes: Last received RSSI
};

// Packed binary structure for received configuration payloads (22 bytes)
struct __attribute__((packed)) ConfigPayload {
  char     header[2];      // 2 bytes: "CF" identifier to reject noise
  uint8_t  targetMac[6];   // 6 bytes: Target MAC address (or FF:FF:FF:FF:FF:FF for broadcast)
  uint32_t networkKey;     // 4 bytes: Shared secret key to prevent unauthorized spoofing
  uint32_t sleepInterval;  // 4 bytes: Sleep interval in seconds
  uint8_t  configVersion;  // 1 byte:  Configuration version
  uint8_t  isDevMode;      // 1 byte:  0 = OP Mode, 1 = Dev Mode
  uint32_t timeOffset;     // 4 bytes: Seconds since CUSTOM_EPOCH
};

// flag to indicate that a packet was received
static volatile bool receivedFlag = false;
static String rssi = "0dBm";
static String snr = "0dB";

// Display variables
static float lastTemp = 0.0;
static float lastVcc = 0.0;
static uint8_t lastBatt = 0;
static String lastMac = "Wait...";
static uint16_t msgCount = 0;
static uint32_t lastDisplayUpdate = 0;
static uint8_t screenNum = 0;

void cryptPayload(uint8_t* data, size_t length, uint16_t msgCount) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, AES_NETWORK_KEY, 128); // 128-bit AES
    
    uint8_t iv[16] = {0};
    // Seed the IV with the message counter to ensure a unique key stream per packet
    iv[14] = (msgCount >> 8) & 0xFF;
    iv[15] = msgCount & 0xFF;
    
    uint8_t stream_block[16] = {0};
    size_t nc_off = 0;
    mbedtls_aes_crypt_ctr(&aes, length, &nc_off, iv, stream_block, data, data);
    mbedtls_aes_free(&aes);
}

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void)
{
    // we got a packet, set the flag
    receivedFlag = true;
}

void setup()
{
    setupBoards();

    // Initialize WiFi and NTP in the background (non-blocking)
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    configTime(0, 0, NTP_SERVER); // Timezone 0 ensures UTC offsets match CUSTOM_EPOCH

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

    // set the function that will be called
    // when new packet is received
    radio.setPacketReceivedAction(setFlag);

    /*
    *   Sets carrier frequency.
    *   SX1278/SX1276 : Allowed values range from 137.0 MHz to 525.0 MHz.
    *   SX1268/SX1262 : Allowed values are in range from 150.0 to 960.0 MHz.
    *   SX1280        : Allowed values are in range from 2400.0 to 2500.0 MHz.
    *   LR1121        : Allowed values are in range from 150.0 to 960.0 MHz, 1900 - 2200 MHz and 2400 - 2500 MHz. Will also perform calibrations.
    * * * */

    if (radio.setFrequency(CONFIG_RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        while (true);
    }

    /*
    *   Sets LoRa link bandwidth.
    *   SX1278/SX1276 : Allowed values are 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250 and 500 kHz. Only available in %LoRa mode.
    *   SX1268/SX1262 : Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
    *   SX1280        : Allowed values are 203.125, 406.25, 812.5 and 1625.0 kHz.
    *   LR1121        : Allowed values are 62.5, 125.0, 250.0 and 500.0 kHz.
    * * * */
    if (radio.setBandwidth(CONFIG_RADIO_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        while (true);
    }


    /*
    * Sets LoRa link spreading factor.
    * SX1278/SX1276 :  Allowed values range from 6 to 12. Only available in LoRa mode.
    * SX1262        :  Allowed values range from 5 to 12.
    * SX1280        :  Allowed values range from 5 to 12.
    * LR1121        :  Allowed values range from 5 to 12.
    * * * */
    if (radio.setSpreadingFactor(9) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        while (true);
    }

    /*
    * Sets LoRa coding rate denominator.
    * SX1278/SX1276/SX1268/SX1262 : Allowed values range from 5 to 8. Only available in LoRa mode.
    * SX1280        :  Allowed values range from 5 to 8.
    * LR1121        :  Allowed values range from 5 to 8.
    * * * */
    if (radio.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        while (true);
    }

    /*
    * Sets LoRa sync word.
    * SX1278/SX1276/SX1268/SX1262/SX1280 : Sets LoRa sync word. Only available in LoRa mode.
    * * */
    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        while (true);
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
        Serial.println(F("Selected output power is invalid for this module!"));
        while (true);
    }

#if !defined(USING_SX1280) && !defined(USING_LR1121) && !defined(USING_SX1280PA)
    /*
    * Sets current limit for over current protection at transmitter amplifier.
    * SX1278/SX1276 : Allowed values range from 45 to 120 mA in 5 mA steps and 120 to 240 mA in 10 mA steps.
    * SX1262/SX1268 : Allowed values range from 45 to 120 mA in 2.5 mA steps and 120 to 240 mA in 10 mA steps.
    * NOTE: set value to 0 to disable overcurrent protection
    * * * */
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));
        while (true);
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
        Serial.println(F("Selected CRC is invalid for this module!"));
        while (true);
    }

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
    // check if the flag is set
    if (receivedFlag) {

        // reset flag
        receivedFlag = false;

        int numBytes = radio.getPacketLength();
        byte byteArr[256];
        int state = radio.readData(byteArr, numBytes);

        flashLed();

        if (state == RADIOLIB_ERR_NONE) {

            rssi = String(radio.getRSSI()) + "dBm";
            snr = String(radio.getSNR()) + "dB";

            if (numBytes == 15 || numBytes == sizeof(TelemetryPayload)) {
                TelemetryPayload rxPayload;
                memcpy(&rxPayload, byteArr, numBytes);
                
                size_t encryptedLength = numBytes - 8;
                uint8_t* dataPtr = ((uint8_t*)&rxPayload) + 8;
                cryptPayload(dataPtr, encryptedLength, rxPayload.msgCount);
                
                lastTemp = rxPayload.temperature / 100.0f;
                lastVcc = rxPayload.battVoltage / 1000.0f;
                lastBatt = rxPayload.battPercent;
                msgCount = rxPayload.msgCount;
                
                char macStr[18];
                sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                        rxPayload.mac[0], rxPayload.mac[1], rxPayload.mac[2],
                        rxPayload.mac[3], rxPayload.mac[4], rxPayload.mac[5]);
                lastMac = String(macStr);
                
                Serial.println(F("Radio Received packet!"));
                Serial.printf("MAC: %s | Msg: %u\n", macStr, msgCount);
                Serial.printf("Temp: %.1fC | VCC: %.2fV | Batt: %d%%\n", lastTemp, lastVcc, lastBatt);
                if (numBytes == sizeof(TelemetryPayload)) {
                    Serial.printf("DEV Mode -> CPU Temp: %dC | RAM: %uKB | Pow: %dmW\n", 
                                  rxPayload.cpuTemp, rxPayload.freeRam, rxPayload.battPower);
                }

                // Transmit configuration reply back to the sensor
                ConfigPayload txConfig;
                txConfig.header[0] = 'C';
                txConfig.header[1] = 'F';
                memcpy(txConfig.targetMac, rxPayload.mac, 6);
                txConfig.networkKey = NETWORK_KEY;
                txConfig.sleepInterval = 20; // Default sleep interval of 60 seconds
                txConfig.configVersion = rxPayload.configVer; // Mirror version, change to force update
                txConfig.isDevMode = 0;      // 0 = OP Mode
                
                // Get current time from NTP to sync the sensor
                time_t now;
                time(&now);
                if (now > CUSTOM_EPOCH) {
                    txConfig.timeOffset = (uint32_t)(now - CUSTOM_EPOCH);
                } else {
                    txConfig.timeOffset = 0; // 0 = NTP not synced yet
                }

                // Delay briefly to ensure the sensor has finished its TX routine 
                // and fully switched into RX mode to catch our preamble.
                delay(100);

                radio.standby();
                int txState = radio.transmit((uint8_t*)&txConfig, sizeof(ConfigPayload));
                if (txState == RADIOLIB_ERR_NONE) {
                    Serial.println(F("Configuration reply transmitted successfully!"));
                    Serial.printf("Config Sent -> Sleep: %lu | Ver: %u | DevMode: %u | TimeOffset: %lu\n",
                                  (unsigned long)txConfig.sleepInterval, txConfig.configVersion, txConfig.isDevMode, (unsigned long)txConfig.timeOffset);
                } else {
                    Serial.printf("Configuration reply failed, code %d\n", txState);
                }
                
                // radio.transmit() is blocking and triggers the hardware interrupt upon completion.
                // This causes our ISR to set receivedFlag = true. We must clear it to avoid a TX-RX infinite loop.
                receivedFlag = false;
            } else {
                Serial.printf("Received unknown packet of %d bytes\n", numBytes);
            }

            screenNum = 0; // Force switch to data screen on new packet
            lastDisplayUpdate = millis();
            drawMain();

        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));
        } else {
            // some other error occurred
            Serial.print(F("failed, code "));
            Serial.println(state);
        }

        // put module back to listen mode
        radio.startReceive();

    }

    // Rotate screen every 5 seconds
    if (millis() - lastDisplayUpdate > 5000) {
        screenNum = (screenNum + 1) % 2;
        lastDisplayUpdate = millis();
        drawMain();
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
            disp->printf("S:%s R:%s W:%s", snr.c_str(), rssi.c_str(), WiFi.isConnected() ? "OK" : "NC");
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
            time_t now;
            time(&now);
            if (now > CUSTOM_EPOCH) {
                char timeStr[20];
                strftime(timeStr, sizeof(timeStr), "%y-%m-%d %H:%M", localtime(&now));
                disp->print(timeStr);
            } else {
                disp->print("Time: Syncing...");
            }
            
            disp->setCursor(5, 60);
            disp->printf("Msg Cnt: %u", msgCount);
        }
        
        disp->sendBuffer();
    }
}