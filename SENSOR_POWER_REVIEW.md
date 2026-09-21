# Sensor Power and Charging Review

Static review of `modes/Sensor/main.cpp` on 2026-09-21. No hardware current measurements were performed.

## Findings

1. **High: SX1276 remains in standby during ESP32 deep sleep.**
   - `transmitData()` and the receive paths finish with `radio.standby()`.
   - `enterDeepSleep()` starts ESP32 deep sleep without calling `radio.sleep()`.
   - SX1276 standby is approximately 1.6 mA, while radio sleep is approximately 0.2 uA.
   - Call `radio.sleep()` immediately before ESP32 deep sleep and handle any returned error.

2. **High: radio initialization failure leaves the ESP32 awake indefinitely.**
   - `setup()` returns when `radio.begin()` fails.
   - Operation-mode `loop()` is empty, so the device never reaches deep sleep.
   - Enter a long fail-safe sleep and retry on a later wake.

3. **High: the board has no BMS and firmware has no low-voltage policy.**
   - Battery percentage becomes 0 below 3.0 V, but sensing and full-power transmission continue.
   - LilyGO requires a protected lithium-ion battery for the T3 V1.6.1.
   - Add a conservative low-voltage long-sleep/cutoff policy, but do not treat firmware as a substitute for cell protection.

4. **Medium: INA226 stays in continuous-conversion mode during sleep.**
   - The INA226 is initialized but never placed in power-down mode.
   - Typical chip current is about 330 uA operating and 0.5 uA in shutdown.
   - Prefer a triggered reading after wake and power-down before ESP32 deep sleep.

5. **Medium: the DS18B20 conversion can keep the ESP32 awake for 750 ms.**
   - `requestTemperatures()` is blocking.
   - A 12-bit conversion can take 750 ms; 9-bit takes 93.75 ms.
   - Use 9- or 10-bit resolution unless finer resolution is required, or overlap an asynchronous conversion with other initialization.

6. **Medium: an invalid clock forces a receive window on every wake.**
   - `checkForUpdates()` checks for a beacon whenever time is invalid.
   - If the gateway is unavailable, repeated RX windows add avoidable consumption.
   - Add retry backoff after several unsuccessful synchronization attempts.

7. **Medium: charge and power telemetry do not match the documentation.**
   - The firmware takes one INA226 snapshot before TX; it does not average readings during TX and RX.
   - Negative current is assumed to mean charging, but polarity depends on shunt orientation.
   - Calculate signed power as bus voltage multiplied by signed current rather than relying on the INA226 power register for reverse flow.
   - Confirm that `setResistorRange(0.1, 1)` matches the installed shunt and calibrate it.
   - Check the result of `ina226.init()` and mark readings invalid when initialization fails.
   - Update the README claim when this behavior is implemented or remove the inaccurate claim separately.

8. **Low: voltage-derived state of charge is unreliable while charging.**
   - The lookup reports 100% at 4.15 V from one instantaneous sample.
   - Charging voltage and load sag can make the percentage misleading.
   - Filter voltage readings and expose charging/discharging state separately.

## Hardware considerations

- The onboard charger is fixed at 500 mA and is not controlled by firmware.
- Use a protected cell rated for at least the board's 500 mA charge current.
- Verify that the INA226 shunt is physically in the battery path. Otherwise it may measure load current but not charging current.
- Radio standby plus INA226 continuous operation alone use about 46 mAh/day. An ideal 2000 mAh cell would last about 43 days before accounting for the ESP32, regulator, display, sensor conversions, TX, or RX.
- Measure sleep current at the battery with USB disconnected after applying fixes. Board regulators, the OLED, charger circuitry, and external sensor modules can dominate the final result.

## Recommended implementation order

1. Use a protected battery and define a low-voltage policy.
2. Put the LoRa radio to sleep before ESP32 deep sleep.
3. Add fail-safe deep sleep when radio initialization fails.
4. Use INA226 triggered conversion and power-down modes.
5. Reduce or overlap DS18B20 conversion time.
6. Add time-sync receive backoff.
7. Calibrate and validate charge telemetry.

## References

- [LilyGO T3 V1.6.1 hardware documentation](https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/docs/en/t3_v161_sx1276/t3_v161_sx1276_hw.md)
- [RadioLib SX1276 API](https://jgromes.github.io/RadioLib/class_s_x1276.html)
- [Semtech SX1276 product documentation](https://www.semtech.com/products/wireless-rf/lora-connect/sx1276)
- [Texas Instruments INA226 datasheet](https://www.ti.com/lit/ds/symlink/ina226.pdf)
- [Analog Devices DS18B20 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf)
