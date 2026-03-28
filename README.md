# lora32_boiler_control

## Configuration

The sensor node can be configured remotely by sending a specific string over LoRa. The device listens for this configuration message for 5 seconds on startup (and every 10 sleep cycles).

### Configuration String Format

The configuration string must follow this format:

`CONFIG:<sleepInterval>,<version>`

-   `CONFIG:`: A required prefix.
-   `<sleepInterval>`: The time in seconds that the device will deep sleep between sensor readings. This must be an integer value.
-   `<version>`: A version number for the configuration. This must be an integer value.

**Example:**

To set the sleep interval to 300 seconds (5 minutes) with a configuration version of 2, you would send the following string:

`CONFIG:300,2`