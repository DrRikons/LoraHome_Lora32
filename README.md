# lora32_boiler_control

## Configuration

The sensor node can be configured remotely by sending a tightly packed binary struct over LoRa. The device listens for this configuration message for 5 seconds on startup (and every 10 sleep cycles).

### Configuration Payload Format

The configuration packet must be transmitted as a 12-byte binary `struct` to maximize time-on-air efficiency:

```cpp
struct __attribute__((packed)) ConfigPayload {
  char     header;      // 2 bytes: "CF" identifier to reject noise
  uint32_t sleepInterval;  // 4 bytes: Sleep interval in seconds
  uint8_t  configVersion;  // 1 byte:  Configuration version
  uint8_t  isDevMode;      // 1 byte:  0 = OP Mode, 1 = Dev Mode
  uint32_t timeOffset;     // 4 bytes: Seconds since Jan 1, 2024 (1704067200)
};
```

    *(Note: Uninitialized sensors will default their base time to this epoch before their first synchronization. A Magic Word is used to validate RTC memory integrity across deep sleep cycles).*

*Note: Changing the mode remotely will trigger a soft reset of the ESP32 to cleanly initialize/de-initialize power-heavy peripherals. If the physical `DEV_MODE_PIN` (GPIO13) is pulled LOW, it acts as a hard hardware override and the device will ignore any remote commands to enter Operation Mode.*

## Project Structure

The project is divided into two main modes:

-   `GateWay`: The gateway node that receives data from the sensor nodes.
-   `Sensor`: The sensor node that reads the boiler temperature and sends it to the gateway.

The main application file for the sensor node is `modes/Sensor/main.cpp`.

*Note: All functions in `main.cpp` are documented inline to indicate whether they are executed in normal Operation mode, Development mode, or both.*

## Telemetry Payload Format
The sensor node transmits telemetry data in the following comma-separated string format:
`ID:<MAC>,T:<Temp>,V:<Volt>,I:<Curr>,P:<Power>,B%:<BattPercent>,S:<ConfigVer>,CNT:<MsgCount>,CT:<CpuTemp>,RAM:<RamKB>,TXP:<TxPwr>,SNR:<SNR>,RSSI:<RSSI>`

*Note: SNR and RSSI metrics represent the signal quality of the last received configuration packet from the gateway.*

## Development
When running in `devMode` (GPIO13 pulled LOW), the sensor will continuously simulate its full operational cycle without deep sleeping: Wake -> Transmit -> Receive -> Display (cycling through all OLED screens) -> Simulated Sleep.

*Note: The code ensures robust parsing and loop execution by matching curly braces cleanly during devMode config testing.*

An `.aiexclude` file is included to prevent AI coding assistants from indexing large third-party libraries in the `lib/` folder and build artifacts in the `.pio/` folder, preserving context space.

For AI assistants, refer to `.github/projectsummary.md` for a high-level overview of the project's architecture, hardware, and configuration formats. Please keep the project summary updated when introducing major changes or new features.