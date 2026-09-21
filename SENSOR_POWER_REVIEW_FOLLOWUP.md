# Sensor Power Review Follow-up

Static follow-up review of the current Sensor firmware on 2026-09-21. This document does not replace `SENSOR_POWER_REVIEW.md`.

Issue 8 from the original review (voltage-derived state-of-charge accuracy) is accepted and intentionally left unchanged.

## Additional optimization opportunities

1. **Resolved: low-battery lockout was evaluated after radio initialization.**
   - INA226 voltage is now checked immediately after sensor initialization.
   - Lockout enters the one-hour sleep before `radio.begin()`; the later check remains as a safeguard.

2. **Resolved: the DS18B20 conversion still blocked for up to 93.75 ms.**
   - Conversion is now asynchronous and overlaps INA226/radio initialization.
   - In development mode, the next conversion overlaps the simulated sleep interval.
   - `readSensors()` waits only for any conversion time that has not already elapsed.

3. **Medium: operation mode always boots and fully reconfigures the radio.**
   - The SX1276 retains registers in radio sleep while the ESP32 deep-sleeps.
   - A retained-state fast path could skip reset and most configuration on timer wakes, with full initialization retained for cold boot and recovery.
   - This requires careful validation because a radio brownout or failed retained state must fall back to full initialization.

4. **Medium: default TX power is the SX1276 maximum configured value of 17 dBm.**
   - TX power dominates the short active-energy peak.
   - Existing remote configuration can lower it, but there is no automatic link-margin policy.
   - Use the lowest site-tested power or add conservative adaptation based on acknowledged downlink RSSI/SNR.

5. **Medium, hardware-dependent: external modules remain electrically powered.**
   - INA226 shutdown reduces the IC current, but breakout-board LEDs and pull-ups can continue drawing power.
   - The DS18B20 and OLED also remain connected to the board supply during ESP32 deep sleep.
   - For lowest sleep current, use load switches or remove module power LEDs. Confirm the result by measuring at the battery with USB disconnected.

6. **Low: an OLED can remain active after switching from development to operation mode.**
   - A soft restart does not remove power from the external OLED.
   - Operation-mode setup does not explicitly send an OLED power-save/display-off command.
   - Explicitly turn the display off before a development-to-operation restart or before operation-mode sleep.

7. **Low: low-battery recovery wakes once per hour indefinitely.**
   - This is safe and allows charger detection, but a deeply depleted unattended node still performs 24 boot cycles per day.
   - Consider progressive retry intervals after repeated low readings, while retaining a reasonably quick first recovery check.

8. **Low: CPU frequency is not reduced during operation-mode work.**
   - Sensor reads and radio waits do not require a 240 MHz CPU.
   - Test 80 or 160 MHz operation, measuring total wake energy rather than current alone because lower frequency can extend execution time.

## Intentionally retained behavior

- An invalid clock checks for a gateway update on every wake.
- Development mode remains continuously awake and display-heavy for diagnostics.
- Voltage-derived battery percentage remains unchanged.

## Suggested priority

1. Measure actual sleep current and eliminate external module/LED loads.
2. Reduce site-specific TX power where link margin permits.
3. Consider retained radio configuration only after hardware measurements justify the added recovery complexity.
