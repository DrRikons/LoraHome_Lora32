# lora32_boiler_control

## Configuration

The sensor node can be configured remotely by sending a specific string over LoRa. The device listens for this configuration message for 5 seconds on startup (and every 10 sleep cycles).

### Configuration String Format

The configuration string must follow this format:

`CONFIG:<sleepInterval>,<version>[,<devMode>[,<timeOffset>]]`

-   `CONFIG:`: A required prefix.
-   `<sleepInterval>`: The time in seconds that the device will deep sleep between sensor readings. This must be an integer value.
-   `<version>`: A version number for the configuration. This must be an integer value.
-   `<devMode>`: *(Optional)* 1 to enable developer mode continuously without deep sleeping, 0 to disable.
-   `<timeOffset>`: *(Optional)* Seconds elapsed since Jan 1, 2024 (1704067200) to synchronize the device's internal RTC.

*Note: Changing the mode remotely will trigger a soft reset of the ESP32 to cleanly initialize/de-initialize power-heavy peripherals. If the physical `DEV_MODE_PIN` (GPIO13) is pulled LOW, it acts as a hard hardware override and the device will ignore any remote commands to enter Operation Mode.*

**Examples:**

- Set sleep interval to 300 seconds, version 2: `CONFIG:300,2`
- Turn on continuous Dev Mode for debugging: `CONFIG:60,3,1`
- Sync time to March 22 2024 (7000000 seconds offset): `CONFIG:60,3,1,7000000`

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
When running in `devMode` (GPIO13 pulled LOW), the sensor will continuously simulate its full operational cycle without deep sleeping: Wake -> Transmit -> Receive -> Display (cycling through all OLED screens, including total TX/RX power-on times) -> Simulated Sleep.

*Note: The codebase uses hardware interrupt-driven, asynchronous RX/TX routines to prevent blocking loops and hangs.*

An `.aiexclude` file is included to prevent AI coding assistants from indexing large third-party libraries in the `lib/` folder and build artifacts in the `.pio/` folder, preserving context space.

For AI assistants, refer to `.github/projectsummary.md` for a high-level overview of the project's architecture, hardware, and configuration formats. Please keep the project summary updated when introducing major changes or new features.