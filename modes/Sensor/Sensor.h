#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "LoRaBoards.h"
#include <RadioLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <INA226_WE.h>

// Pin definitions
#define DS18B20_PIN 4  // GPIO4 for DS18B20
#define DEV_MODE_PIN 13 // GPIO13 for dev mode toggle (pull low to enable)

// Dev mode flag
extern bool devMode;

// Sensor objects
extern OneWire oneWire;
extern DallasTemperature sensors;
extern INA226_WE ina226;

// Data structure for transmission
struct SensorData {
  float temperature;
  float batteryVoltage;
  float batteryCurrent;
  float batteryPower;
  uint32_t timestamp;
  uint8_t configVersion;
};
extern SensorData sensorData;

// Configuration structure
struct Config {
  uint32_t sleepInterval; // seconds
  uint8_t configVersion;
};
extern Config config;

// RTC memory for config persistence
extern RTC_DATA_ATTR Config rtcConfig;

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
extern SX1276 radio;

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
extern SX1278 radio;

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
extern SX1262 radio;

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
extern SX1280 radio;

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
extern SX1280 radio;

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
extern SX1268 radio;

#elif   defined(USING_LR1121)
#define CONFIG_RADIO_FREQ           2450.0
#define CONFIG_RADIO_OUTPUT_POWER   LILYGO_RADIO_2G4_TX_POWER_LIMIT
#define CONFIG_RADIO_BW             125.0
extern LR1121 radio;
#endif

// Function declarations
void setup();
void loop();
void readSensors();
void transmitData();
void enterDeepSleep();
void listenForConfig();
void drawMain();
void setFlag(void);

#endif // SENSOR_H