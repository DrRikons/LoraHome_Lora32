#ifndef PAYLOADS_H
#define PAYLOADS_H

#include <Arduino.h> // For uint8_t, uint16_t, etc.

constexpr uint8_t CONFIG_UPDATE_PENDING_FLAG = 0xA5;

// Packed binary structure for highly efficient LoRa transmission
struct __attribute__((packed)) TelemetryPayload {
  // --- Core variables (16 bytes, always sent) ---
  uint8_t  mac[6];       // 6 bytes: Raw MAC address
  uint16_t msgCount;     // 2 bytes: Message counter (cycles at 65535)
  int16_t  temperature;  // 2 bytes: External Temp (x 100)
  uint16_t battVoltage;  // 2 bytes: Battery Voltage in mV (x 1000)
  uint8_t  battPercent;  // 1 byte:  Battery capacity (0-100%)
  uint8_t  isDevMode;    // 1 byte:  0=Op, 1=Dev
  uint8_t  needsTimeSync; // 1 byte:  0=clock OK, 1=clock unset/needs sync
  uint8_t  sleepInterval; // 1 byte: Applied sleep interval in seconds
  
  // --- Dev mode variables (11 bytes, conditionally sent) ---
  int16_t  battCurrent;  // 2 bytes: Battery Current in mA
  int16_t  battPower;    // 2 bytes: Battery Power in mW
  uint16_t freeRam;      // 2 bytes: Free RAM in KB
  int8_t   cpuTemp;      // 1 byte:  CPU Temp in C
  int8_t   txPower;      // 1 byte:  TX Power in dBm
  int8_t   lastSNR;      // 1 byte:  Last received SNR
  int16_t  lastRSSI;     // 2 bytes: Last received RSSI
};

// Packed binary structure for received configuration payloads (18 bytes)
struct __attribute__((packed)) ConfigPayload {
  char     header[2];      // 2 bytes: "CF" identifier to reject noise
  uint8_t  targetMac[6];   // 6 bytes: Target MAC address (or FF:FF:FF:FF:FF:FF for broadcast)
  uint32_t networkKey;     // 4 bytes: Shared secret key to prevent unauthorized spoofing
  uint8_t  sleepInterval;  // 1 byte: Sleep interval in seconds
  // uint8_t  configVersion;  // 1 byte:  Config sync token
  uint8_t  isDevMode;      // 1 byte:  0 = OP Mode, 1 = Dev Mode
  uint32_t timeOffset;     // 4 bytes: UTC seconds since CUSTOM_EPOCH
};

#endif // PAYLOADS_H
