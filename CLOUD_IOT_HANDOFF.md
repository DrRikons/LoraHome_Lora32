# LoRaHome cloud IoT implementation handoff

## Purpose

Use this document as the source of truth when asking an AI to build the cloud side of this project. The ESP32 Gateway is the edge device: it receives encrypted LoRa packets, converts them to JSON, and publishes them to MQTT. The cloud project must ingest, persist, query, visualize, and alert on that MQTT data. Do not require changes to the LoRa Sensor or Gateway for the initial cloud implementation.

## Existing edge contract

### Connectivity and broker requirements

- MQTT protocol client: PubSubClient on ESP32.
- Endpoint: a bare DNS host and numeric port configured on the Gateway SD card; port defaults to `8883` when omitted or invalid. Do not include a scheme (such as `http://`) or a port suffix in `mqtt.host`.
- Authentication: MQTT username and password from `/config.json` on the SD card.
- Client ID: `LoraHomeGW-<ESP32-efuse-MAC-in-hex>`.
- Topic root: lowercase `lorahome`.
- QoS: default PubSubClient QoS 0. The Gateway does not subscribe to cloud topics.
- Retention: `lorahome/gateway/status` and each `lorahome/sensor/<sensor_mac>/status` message are retained. Sensor `data` and `telemetry` messages are not retained.
- Timestamps: every JSON payload has `timestamp`, formatted `YYYY-MM-DDTHH:MM:SS`, in the Gateway's `Europe/Athens` local time and with no UTC offset. It may be a 1970-era value before NTP synchronization. Treat it as an untrusted device timestamp; record an authoritative ingestion timestamp in UTC.
- Delivery: messages are sent only while the Wi-Fi and broker connection are available. There is no offline queue or replay, so the cloud must tolerate gaps and duplicate messages.

The Gateway currently creates a TLS socket but calls `setInsecure()`: encryption is used, but the broker certificate is not verified. For production, plan a firmware follow-up that installs and validates the broker CA certificate.

### Gateway configuration file

The Gateway reads `/config.json` from its SD card. The cloud deployment supplies the `mqtt` values:

```json
{
  "wifi": { "ssid": "SITE_WIFI", "password": "SITE_WIFI_SECRET" },
  "mqtt": { "host": "mqtt.example.net", "port": 8883, "user": "gateway-site-01", "password": "LONG_RANDOM_SECRET" },
  "sensor": { "defaultSleepSeconds": 20, "defaultDevMode": true, "defaultTxPower": 17 }
}
```

Never place real credentials in source control, Grafana dashboards, documentation, or AI prompts.

## MQTT topics and schemas

Topic names are case-sensitive. `<sensor_mac>` is uppercase, colon-separated hexadecimal (for example, `24:6F:28:12:34:56`).

| Topic | Producer behavior | JSON fields |
| --- | --- | --- |
| `lorahome/sensor/<sensor_mac>/data` | One message for every valid 21-byte or 31-byte sensor packet; non-retained. | `timestamp`, `mac`, `bootNonce`, `msgCount`, `temperature`, `battVoltage`, `battPercent`, `isDevMode`, `needsTimeSync`, `sleepInterval`, `txPower` |
| `lorahome/sensor/<sensor_mac>/telemetry` | Additional message only when the received sensor packet says development mode; non-retained. | `timestamp`, `battCurrent`, `battPower`, `freeRam`, `cpuTemp`, `txPower`, `lastSNR`, `lastRSSI` |
| `lorahome/sensor/<sensor_mac>/status` | Retained after every valid sensor packet, when the Gateway reconnects to MQTT, and when the sensor becomes offline. The Gateway marks it offline after three missed advertised sleep intervals plus a 5-second grace period. | `timestamp`, `online`, `uptime` |
| `lorahome/gateway/status` | On MQTT connection, then on meaningful status changes no more often than 5 seconds, plus a 60-second heartbeat; retained. | `timestamp`, `gatewayRssi`, `gatewaySnr`, `wifiStatus`, `ip`, `freeRam` |

Example core telemetry:

```json
{
  "timestamp": "2026-09-16T14:03:21",
  "mac": "24:6F:28:12:34:56",
  "bootNonce": 1234567890,
  "msgCount": 42,
  "temperature": 2187,
  "battVoltage": 3712,
  "battPercent": 78,
  "isDevMode": 0,
  "needsTimeSync": 0,
  "sleepInterval": 20,
  "txPower": 17
}
```

Example development telemetry:

```json
{
  "timestamp": "2026-09-16T14:03:21",
  "battCurrent": -18,
  "battPower": -67,
  "freeRam": 142,
  "cpuTemp": 36,
  "txPower": 17,
  "lastSNR": 7,
  "lastRSSI": -89
}
```

Example sensor status:

```json
{
  "timestamp": "2026-09-16T14:03:21",
  "online": true,
  "uptime": 1260
}
```

## Sensor configuration command contract

The cloud control API publishes non-retained QoS 1 commands to:

```text
lorahome/sensor/<sensor_mac>/config
```

`<sensor_mac>` is the uppercase colon-separated MAC address already used in the
sensor data topics. The Gateway must subscribe to `lorahome/sensor/+/config`,
verify that the topic MAC identifies a known sensor, and relay or apply only
validated configuration for that sensor.

The command payload is JSON:

```json
{
  "commandId": "uuid",
  "schemaVersion": 1,
  "expiresAt": "2026-09-17T12:00:00+00:00",
  "config": {
    "defaultSleepSeconds": 60,
    "defaultDevMode": false
  },
  "signature": "hex-hmac-sha256"
}
```

`config` must contain at least one field. `defaultSleepSeconds` is an integer
from 10 through 86400; `defaultDevMode` is boolean. Reject unknown fields,
expired commands, invalid topic MACs, and duplicate `commandId` values.

The signature is HMAC-SHA256 using the shared `CONTROL_COMMAND_SECRET`. Compute
it over UTF-8 JSON containing only `commandId`, `schemaVersion`, `expiresAt`, and
`config`, serialized with lexicographically sorted keys and compact separators
(`,` and `:`). Compare against the lowercase hexadecimal `signature` value using
a constant-time comparison. Never log the shared secret.

After processing, publish a non-retained QoS 1 result to:

```text
lorahome/sensor/<sensor_mac>/config/result
```

```json
{
  "commandId": "uuid",
  "status": "applied"
}
```

Valid result statuses are `applied`, `rejected`, and `expired`. The cloud uses
this response to update the command status displayed through its control API.

## Field semantics and normalization

The telemetry values below are raw values emitted by the current firmware. Preserve raw telemetry values, then derive engineering units in the ingestion service or query layer. Sensor-status fields are Gateway-derived.

| Field | Type | Meaning / unit | Cloud normalization |
| --- | --- | --- | --- |
| `mac` | string | Sensor identity. The raw binary MAC array is replaced with this string in core JSON. | Use as `sensor_id` tag/key. |
| `bootNonce` | unsigned integer | Random per-boot sensor nonce. | Pair with `mac` and `msgCount` for a best-effort packet identity. |
| `msgCount` | unsigned integer | Incrementing sensor message counter; may reset when the sensor reboots. | Store as integer; do not assume global monotonicity. |
| `temperature` | signed integer | Celsius multiplied by 100. | `temperature_c = temperature / 100.0`. |
| `battVoltage` | unsigned integer | Battery volts multiplied by 1000. | `battery_voltage_v = battVoltage / 1000.0`. |
| `battPercent` | unsigned integer | Battery percentage. | Store as percent. |
| `isDevMode` | integer flag | Sensor development mode (`0`/`1`). | Boolean/flag. |
| `needsTimeSync` | integer flag | Sensor clock needs synchronization (`0`/`1`). | Boolean/flag and alert signal. |
| `sleepInterval` | unsigned integer | Configured sensor sleep period in seconds. | Store as seconds. |
| `online` | boolean | Gateway's current view of sensor presence. It becomes `false` after three missed advertised sleep intervals plus 5 seconds. | Store as the latest retained sensor-status value. Treat it as Gateway-observed, not a sensor self-report. |
| `uptime` | unsigned integer | Seconds the Gateway has continuously considered this sensor online since its first packet or recovery from offline. | Store as seconds; it is `0` while offline and resets when the Gateway restarts. |
| `battCurrent` | signed integer | INA226 current in mA, truncated to an integer. Negative is possible. | `battery_current_ma`. |
| `battPower` | signed integer | INA226 power in mW, truncated to an integer. | `battery_power_mw`. |
| `freeRam` | unsigned integer | Sensor free RAM in KiB on development telemetry; Gateway free heap in bytes on status. | Keep separate measurement/field names to avoid unit collision. |
| `cpuTemp` | signed integer | ESP32 CPU temperature in degrees C. | `cpu_temperature_c`. |
| `txPower` | signed integer | LoRa transmit power in dBm. | Store as dBm. |
| `lastSNR`, `lastRSSI` | signed integer | Sensor's last received configuration-downlink signal quality, dB/dBm. | Store separately from gateway radio signal. |
| `gatewayRssi`, `gatewaySnr` | integer/float | Gateway's current radio RSSI/SNR, dBm/dB. It is not explicitly bound to a specific sensor packet. | Store as gateway health metrics. |
| `wifiStatus`, `ip` | string | ESP32 Wi-Fi state and local IP address. | Gateway status metadata. |

Important: development telemetry has no `mac`, `msgCount`, or `bootNonce`. Join it to core telemetry by MQTT arrival time and the most recent core message for a sensor only as a best-effort convenience. The better future firmware contract is to include `mac`, `bootNonce`, and `msgCount` in development telemetry.

## Recommended initial cloud architecture

```text
ESP32 Gateway -> TLS MQTT broker -> MQTT consumer/normalizer -> InfluxDB
                          |                    |                 |
                          |                    +-> dead-letter / logs
                          +-> Grafana alert state <------------- Grafana
```

1. Run a managed MQTT broker or Mosquitto/EMQX with a public TLS endpoint on port 8883.
2. Create a dedicated credential for each gateway, restricted to publish only beneath `lorahome/` (ideally a gateway-specific namespace in a later firmware revision).
3. Run a durable MQTT consumer using a stable client ID and subscribe to `lorahome/#`. Validate payloads and topics, add `ingested_at` in UTC, normalize units, and write to InfluxDB 2.x/3.x.
4. Use Grafana with InfluxDB as the datasource. Build dashboards for sensor temperature/battery, packet cadence, gateway connectivity, and diagnostics.
5. Send alerts through Grafana Alerting (email, webhook, or another approved channel). Do not send commands to the Gateway initially: inbound MQTT configuration is deliberately disabled in firmware.

For a single gateway, Telegraf's MQTT consumer plus InfluxDB is a valid small deployment. A custom normalizer is preferable if it must correlate dev/core messages, enforce schemas, deduplicate, or route tenant/site metadata.

## Storage model

Use a device inventory outside the time-series stream (PostgreSQL, SQLite, or a versioned configuration file) to map `sensor_id` to display name, site, room, boiler zone, and expected reporting interval. Do not encode mutable friendly names in MQTT topics.

Suggested InfluxDB measurements:

| Measurement | Tags | Fields |
| --- | --- | --- |
| `sensor_reading` | `sensor_id`, `site` | `temperature_c`, `battery_voltage_v`, `battery_percent`, `sleep_interval_s`, `is_dev_mode`, `needs_time_sync`, raw counter/nonce |
| `sensor_diagnostics` | `sensor_id`, `site` | current/power, RAM KiB, CPU temperature, TX power, last downlink SNR/RSSI |
| `sensor_status` | `sensor_id`, `site` | online flag and continuous sensor online duration in seconds |
| `gateway_status` | `gateway_id`, `site`, `wifi_status` | radio RSSI/SNR, free heap bytes, IP as optional field |
| `ingestion_event` | `gateway_id`, `sensor_id`, `reason` | malformed/unknown-topic counters; retain short-term only |

Use `ingested_at` (UTC) as the primary write timestamp. Optionally preserve parsed `gateway_reported_at` as a field after attaching the `Europe/Athens` timezone, but reject or flag it if it is before 2024 or materially differs from receipt time. This avoids false historical points when NTP is unavailable.

## Initial dashboards and alerts

- Boiler/sensor overview: latest temperature, battery voltage/percent, time since last core packet, and `needsTimeSync`.
- Sensor presence: latest retained `online` state; the Gateway declares offline after three missed advertised sleep intervals plus five seconds.
- Per-sensor history: temperature over time, expected versus observed reporting interval, and battery trend.
- Gateway health: retained status age, Wi-Fi state, free heap, radio RSSI/SNR, and MQTT consumer lag/errors.
- Development diagnostics: shown only for sensors in development mode; keep them out of primary operational dashboards.
- Alert when no core telemetry arrives for `max(3 * sleepInterval, configured minimum) + delivery grace`; start with a 10-minute minimum until a device inventory is established.
- Alert on low battery using a site-configured threshold, sustained `needsTimeSync = 1`, gateway status older than 2 minutes, Wi-Fi state other than `CONNECTED`, or malformed-message bursts.

## Security and operations requirements

- Require TLS 1.2+ on the broker. Plan the Gateway CA-validation firmware update before declaring the deployment production-grade.
- Use unique, revocable MQTT credentials per gateway; never use an admin or shared dashboard credential on hardware.
- Apply broker ACLs, rate limits, audit logs, secret rotation, database retention, backups, and monitoring.
- Treat all MQTT payloads as untrusted input. Check topic shape, JSON types, numeric ranges, and maximum payload size before writing.
- The source schema has no version field. Version the cloud normalizer's accepted schema and introduce a `schemaVersion` field in a future firmware update before making breaking changes.

## Prompt for a future implementation AI

> Build the cloud backend described in `CLOUD_IOT_HANDOFF.md` for the existing LoRaHome ESP32 Gateway. Do not change firmware in the first phase. Provision a TLS MQTT broker, a durable MQTT-to-InfluxDB ingestion service, Grafana dashboards, and Grafana alerts. Subscribe to the exact lowercase `lorahome/#` topic contract and preserve raw values while deriving engineering-unit fields. Consume retained sensor-status messages as Gateway-observed presence, including their continuous online duration. Use UTC ingestion time as the database timestamp because the device timestamp is Athens local time without an offset and can be invalid before NTP sync. Implement strict validation, observability, credentials via environment/secrets management, Docker Compose for local development, and deployment documentation. Include automated tests with the sample payloads in this document. Do not implement cloud-to-device commands because the Gateway does not subscribe to MQTT.

## Known interface limitations to plan around

- No MQTT QoS 1, persistent session, offline buffering, or replay: packet loss is expected.
- No explicit gateway ID or site ID in MQTT topics/payloads. A single broker serving multiple gateways can collide when sensors share a MAC-derived topic; add a gateway/site namespace in a coordinated future firmware and cloud migration.
- The status topic is global (`lorahome/gateway/status`) and retained, so it supports only one Gateway without topic changes.
- Sensor presence is held only in a 16-entry Gateway RAM table. A Gateway restart cannot mark previously retained sensor statuses offline; treat their timestamps and the Gateway status age as staleness signals until the sensor transmits again.
- Core and development messages are published separately and not transactionally.
- The MQTT buffer is 256 bytes. Keep payload additions small or update the firmware buffer deliberately.
