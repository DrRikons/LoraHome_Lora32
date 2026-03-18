#include "Sensor.h"

// Dev mode flag
bool devMode = true;

// Sensor objects
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
INA226_WE ina226 = INA226_WE(0x40); // INA226 at default I2C address 0x40

// Data structure for transmission
SensorData sensorData;

// Configuration structure
Config config = {20, 1}; // Default 20 seconds

// RTC memory for config persistence
RTC_DATA_ATTR Config rtcConfig;

#if     defined(USING_SX1276)
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);
#elif   defined(USING_SX1278)
SX1278 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);
#elif   defined(USING_SX1262)
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#elif   defined(USING_SX1280)
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#elif  defined(USING_SX1280PA)
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#elif   defined(USING_SX1268)
SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#elif   defined(USING_LR1121)
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


// save transmission state between loops
static int transmissionState = RADIOLIB_ERR_NONE;
// flag to indicate that a packet was sent
static volatile bool transmittedFlag = false;
static uint32_t counter = 0;
static String payload;

// Transmission details
static String deviceId;
static int screenNum = -1;
static int msgOffset = 0;

void setFlag(void)
{
    // we sent a packet, set the flag
    transmittedFlag = true;
}

void setup()
{
    // Check dev mode
    pinMode(DEV_MODE_PIN, INPUT_PULLUP);
    devMode = (digitalRead(DEV_MODE_PIN) == LOW);

    if (devMode) {
        setupBoards(); // Enable all peripherals for dev mode
        Serial.println("Dev/Debug mode");
    } else {
        // Minimal setup for power saving
        Serial.begin(115200);
        delay(100);
        Serial.println("Operation mode");
        
        // Disable WiFi and Bluetooth
        WiFi.mode(WIFI_OFF);
        btStop();
        
        // Initialize I2C only for sensors
        Wire.begin(21, 22); // SDA, SCL
        
        // Initialize display if available
        #ifdef HAS_DISPLAY
        beginDisplay();
        #endif
        
        // Initialize SPI for radio
        SPI.begin(5, 19, 27);
        
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
    ina226.init(); // Initialize INA226
    ina226.setResistorRange(0.1, 1); // 0.1 ohm shunt, range 1

    // Load config from RTC memory
    config = rtcConfig;
    if (config.sleepInterval < 10 || config.sleepInterval > 86400) {
        // Protect against invalid or uninitialized RTC values
        config.sleepInterval = 60;
        config.configVersion = 1;
    }

    // Radio setup (same as before, but only if not sleeping)
    int state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("Radio init failed: "));
        Serial.println(state);
        return;
    }

    // Set radio parameters (same as before)
    radio.setFrequency(CONFIG_RADIO_FREQ);
    radio.setBandwidth(CONFIG_RADIO_BW);
    radio.setSpreadingFactor(12);
    radio.setCodingRate(6);
    radio.setSyncWord(0xAB);
    radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER);
    radio.setCRC(false);

    // Set packet sent callback
    radio.setPacketSentAction(setFlag);

    // Display mode message
    #ifdef HAS_DISPLAY
    if (disp) {
        // Unique device identity (use MAC)
        uint64_t mac = ESP.getEfuseMac();
        char idBuf[17];
        sprintf(idBuf, "%08X%08X", (uint32_t)(mac >> 32), (uint32_t)mac);
        deviceId = String(idBuf);

        const char *modeText = devMode ? "DEV MODE" : "OP MODE";
        disp->clearBuffer();
        disp->setFont(u8g2_font_fur11_tf);
        int16_t x = (disp->getDisplayWidth() - disp->getUTF8Width(modeText)) / 2;
        disp->drawStr(x, 30, modeText);
        disp->sendBuffer();
        delay(2000);
        disp->clearBuffer();
        disp->sendBuffer();
    }
    #endif

    // Read sensors and transmit
    readSensors();
    if (devMode) {
        drawMain();
    }
    transmitData();

    // Enter deep sleep
    enterDeepSleep();
}

void loop()
{
    // In operation mode, we don't loop - we transmit and sleep
    // In dev mode, we can add debug functionality here
    if (devMode) {
        // Dev mode: keep transmitting periodically for testing
        readSensors();
        drawMain();
        transmitData();
        delay(4000); // 4 second interval in dev mode
    }
    // Otherwise, loop does nothing as we sleep after setup
}


void readSensors()
{
    // Read DS18B20 temperature
    sensors.requestTemperatures();
    sensorData.temperature = sensors.getTempCByIndex(0);

    // Read INA226 data
    sensorData.batteryVoltage = ina226.getBusVoltage_V();
    sensorData.batteryCurrent = ina226.getCurrent_mA();
    sensorData.batteryPower = ina226.getBusPower();

    // Set timestamp (placeholder - would get from gateway)
    sensorData.timestamp = millis();
    sensorData.configVersion = config.configVersion;

    if (devMode) {
        Serial.printf("Temp: %.2f C, Volt: %.2f V, Curr: %.2f mA, Power: %.2f mW\n",
                      sensorData.temperature, sensorData.batteryVoltage,
                      sensorData.batteryCurrent, sensorData.batteryPower);
    }
}

void transmitData()
{
    // Prepare payload (includes unique device ID and message counter)
    uint32_t msgCount = ++counter;
    payload = "ID:" + deviceId + ",T:" +
              String(sensorData.temperature, 1) + ",V:" +
              String(sensorData.batteryVoltage, 2) + ",I:" +
              String(sensorData.batteryCurrent, 1) + ",P:" +
              String(sensorData.batteryPower, 1) + ",S:" +
              String(sensorData.configVersion) + ",CNT:" +
              String(msgCount);

    // Turn LED on during transmission
    digitalWrite(BOARD_LED, LED_ON);

    transmissionState = radio.startTransmit(payload.c_str());

    if (devMode) {
        Serial.print("Transmitting: ");
        Serial.println(payload);
    }

    // Wait for transmission to complete
    while (!transmittedFlag) {
        delay(10);
    }
    transmittedFlag = false;

    // Turn LED off after transmission
    digitalWrite(BOARD_LED, !LED_ON);

    if (devMode) {
        Serial.println("Transmission complete");
    }
}

void enterDeepSleep()
{
    // Listen briefly for config (every 10 cycles)
    static uint8_t cycleCount = 0;
    cycleCount++;
    if (cycleCount >= 10) {
        cycleCount = 0;
        listenForConfig();
    }

    if (devMode) {
        Serial.printf("Entering deep sleep for %d seconds\n", config.sleepInterval);
        delay(1000); // Allow serial to finish
        return; // Don't sleep in dev mode
    }

    // Configure wake up timer
    esp_sleep_enable_timer_wakeup(config.sleepInterval * 1000000ULL); // microseconds

    // Enter deep sleep
    esp_deep_sleep_start();
}

void listenForConfig()
{
    radio.startReceive();
    unsigned long startTime = millis();
    while (millis() - startTime < 5000) { // Listen for 5 seconds
        if (radio.available()) {
            String received = "";
            int state = radio.readData(received);
            if (state == RADIOLIB_ERR_NONE && received.startsWith("CONFIG:")) {
                // Parse config: CONFIG:sleepInterval,version
                int commaIndex = received.indexOf(',');
                if (commaIndex > 0) {
                    config.sleepInterval = received.substring(7, commaIndex).toInt();
                    config.configVersion = received.substring(commaIndex + 1).toInt();
                    rtcConfig = config; // Save to RTC
                    if (devMode) {
                        Serial.printf("Config updated: sleep=%d, version=%d\n",
                                      config.sleepInterval, config.configVersion);
                    }
                }
            }
        }
        delay(10);
    }
    radio.standby();
}

void drawMain()
{
    if (devMode) {
        Serial.println("drawMain called");
        if (disp) {
            Serial.println("disp is not null, drawing...");
        } else {
            Serial.println("disp is null, cannot draw");
        }
    }
    if (devMode && disp) {
        screenNum = (screenNum + 1) % 2;
        disp->clearBuffer();
        disp->drawRFrame(0, 0, 128, 64, 5);
        disp->setFont(u8g2_font_pxplusibmvga8_mr);
        if (screenNum == 0) {
            // Sensor data screen
            disp->setCursor(5, 15);
            disp->printf("Temp: %.1f C", sensorData.temperature);
            disp->setCursor(5, 30);
            disp->printf("Volt: %.2f V", sensorData.batteryVoltage);
            disp->setCursor(5, 45);
            disp->printf("Curr: %.0f mA", sensorData.batteryCurrent);
            disp->setCursor(5, 60);
            disp->printf("Sleep: %d s", config.sleepInterval);
        } else {
                // Message preview (scrolling)
            if (payload.length() > 10) {
                msgOffset = (msgOffset + 1) % (payload.length() - 10);
            } else {
                msgOffset = 0;
            }
            disp->setCursor(5, 15);
            disp->printf("Msg: %.10s", payload.c_str() + msgOffset);
            disp->setCursor(5, 30);
            disp->printf("ID: %.6s", deviceId.c_str());
            disp->setCursor(5, 45);
            disp->printf("Cnt: %u", counter);
            disp->setCursor(5, 60);
            disp->printf("Ver: %d", config.configVersion);
        }
        disp->sendBuffer();
    }
}