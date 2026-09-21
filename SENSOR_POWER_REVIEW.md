# Sensor Power and Charging Review

Static review of `modes/Sensor/main.cpp` on 2026-09-21. No hardware current measurements were performed.

## Findings

1. **Resolved: SX1276 remained in standby during ESP32 deep sleep.**
   - Previously, `transmitData()` and the receive paths finished with `radio.standby()`.
   - Previously, `enterDeepSleep()` started ESP32 deep sleep without calling `radio.sleep()`.
   - SX1276 standby is approximately 1.6 mA, while radio sleep is approximately 0.2 uA.
   - Implemented `radio.sleep()` immediately before ESP32 deep sleep with error reporting.

2. **Resolved: radio initialization failure left the ESP32 awake indefinitely.**
   - Previously, `setup()` returned when `radio.begin()` failed.
   - Because operation-mode `loop()` is empty, the device did not reach deep sleep.
   - Implemented a five-minute fail-safe deep sleep before retrying initialization.

3. **Resolved in firmware: the board has no BMS and previously had no low-voltage policy.**
   - At 3.2 V or below, firmware now suppresses radio TX/RX and enters a one-hour deep sleep.
   - A retained lockout requires recovery to 3.4 V before normal operation resumes, preventing rapid cycling near the threshold.
   - The policy also overrides development mode.
   - LilyGO requires a protected lithium-ion battery for the T3 V1.6.1.
   - Firmware is not a substitute for cell protection; a protected battery remains mandatory.

4. **Resolved: INA226 stayed in continuous-conversion mode during sleep.**
   - Previously, the INA226 was initialized but never placed in power-down mode.
   - Typical chip current is about 330 uA operating and 0.5 uA in shutdown.
   - Implemented INA226 power-down before ESP32 deep sleep. Deep-sleep wake resets and initializes it for the next reading.

5. **Resolved: the DS18B20 conversion kept the ESP32 awake for up to 750 ms.**
   - `requestTemperatures()` remains blocking, but the sensor now uses 9-bit resolution.
   - Temperature has 0.5 C steps, is displayed with one decimal (for example `45.5`), and conversion takes at most 93.75 ms.

6. **Accepted by design: an invalid clock forces a receive window on every wake.**
   - `checkForUpdates()` checks for a beacon whenever time is invalid.
   - If the gateway is unavailable, repeated RX windows add avoidable consumption.
   - This behavior is intentionally retained so an unsynchronized Sensor checks for gateway time on every wake.

7. **Resolved: charge and power telemetry did not match the documentation.**
   - Development mode now samples INA226 voltage and signed current every 20 ms while asynchronous LoRa TX/RX windows are active.
   - The completed average is sent in `battPower` on the next development telemetry cycle; the first cycle uses the idle reading.
   - Signed power is calculated as bus voltage multiplied by signed current instead of using the INA226 power register.
   - The display reports signed `I` and `P` values without assuming that negative current always means charging.
   - Shunt resistance, current range, and direction are explicit firmware constants and must match the installed wiring.
   - INA226 initialization failure is checked and produces zeroed battery readings instead of unchecked I2C data.

8. **Low: voltage-derived state of charge is unreliable while charging.**
   - The lookup reports 100% at 4.15 V from one instantaneous sample.
   - Charging voltage and load sag can make the percentage misleading.
   - Filter voltage readings and expose charging/discharging state separately.

## Hardware considerations

- The onboard charger is fixed at 500 mA and is not controlled by firmware.
- Use a protected cell rated for at least the board's 500 mA charge current.
- Verify that the INA226 shunt is physically in the battery path. Otherwise it may measure load current but not charging current.
- Before issues 1 and 4 were fixed, radio standby plus INA226 continuous operation alone used about 46 mAh/day.
- Measure sleep current at the battery with USB disconnected after applying fixes. Board regulators, the OLED, charger circuitry, and external sensor modules can dominate the final result.

## Remaining recommendations

1. Use a protected battery despite the firmware low-voltage policy.
2. Verify the INA226 shunt resistance and current direction against the installed wiring.
3. Filter or otherwise improve voltage-derived state-of-charge reporting.

## References

- [LilyGO T3 V1.6.1 hardware documentation](https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/docs/en/t3_v161_sx1276/t3_v161_sx1276_hw.md)
- [RadioLib SX1276 API](https://jgromes.github.io/RadioLib/class_s_x1276.html)
- [Semtech SX1276 product documentation](https://www.semtech.com/products/wireless-rf/lora-connect/sx1276)
- [Texas Instruments INA226 datasheet](https://www.ti.com/lit/ds/symlink/ina226.pdf)
- [Analog Devices DS18B20 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf)
