# Copilot Instructions for Lora32_boiler_control

Trust these instructions first. Only search the codebase if information here is missing or
demonstrably incorrect — this saves significant exploration time.

## Repository Summary
Embedded C++ (Arduino/PlatformIO) firmware for an ESP32-based LoRa boiler monitoring system.
Two firmware images are built from one codebase:
- **Sensor** (`modes/Sensor/main.cpp`): reads temperature (DS18B20) and battery (INA226) data,
  encrypts it (AES-128-CTR), transmits via LoRa (RadioLib), then deep-sleeps.
- **GateWay** (`modes/GateWay/main.cpp`, legacy `GateWay.ino` also present): receives/decrypts
  sensor packets, connects to WiFi/MQTT/NTP, drives an OLED display, sends config downlinks.

Shared payload structs live in `payloads.h` (per mode dir). Repo is small (~dozens of source
files); the vast majority of the tree under `lib/` is vendored third-party Arduino libraries —
**do not read or modify anything under `lib/` unless a task explicitly requires patching a
vendored library**; treat it as read-only, pre-existing dependency code.

For deeper architecture/hardware/protocol details, read [`.github/projectsummary.md`](.github/projectsummary.md)
and [`README.md`](../README.md) before making non-trivial changes — they are kept up to date and
contain protocol formats (config/telemetry payload byte layouts), power-saving RX/TX state
machine behavior, and dev-mode simulation logic. Update both files when changing logic per the
rules in `AGENTS.md` (do NOT update README for pure syntax fixes).

## Toolchain / Environment
- Build system: **PlatformIO Core 6.2.0**, framework `arduino`, platform `espressif32@6.12.0`.
- PlatformIO CLI is **not on PATH** in a fresh shell. It is installed at:
  `$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe` (invoke via full path, or add to PATH
  for the session with `$env:PATH += ";$env:USERPROFILE\.platformio\penv\Scripts"`).
- C/C++ compilers (gcc/g++) are **not installed** in the base environment — required only for the
  `native` unit-test env (see Testing below).

## Build (always works, validated)
```
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e T3_V1_6_SX1276
```
- Builds whichever mode is set via `src_dir` in `platformio.ini` (currently `modes/GateWay`; the
  `modes/Sensor` line is present but commented out — swap by editing `src_dir` if the task
  requires building the Sensor firmware instead).
- Takes ~25 seconds once the espressif32 toolchain/platform packages are already cached locally
  (they are, in this environment — no network download occurs).
- **Known failure and required fix**: on a completely fresh machine this build fails with
  `ModuleNotFoundError: No module named 'intelhex'` while generating `bootloader.bin`. Fix by
  running once:
  ```
  & "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" -m pip install intelhex
  ```
  After that, the build succeeds cleanly with no warnings (validated: RAM ~15%, Flash ~79% used).
- There is no other env defined for the sensor board target besides `T3_V1_6_SX1276`; `native` is
  test-only (see below) and is NOT a firmware build target.

## Testing
Unit tests live under `test/` (`test_sensor`, `test_gateway`, `test_end_to_end`, plus shared
support code in `test/support/LoraHomeProtocolModel.h`) and run via PlatformIO's `native` platform
(Unity test framework), defined in the `[env:native]` section of `platformio.ini`.
```
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" test -e native
```
- **This requires a real gcc/g++ compiler on PATH** (MinGW-w64/MSYS2 or similar). It is **not
  present by default** in this environment and the command fails with
  `'gcc' is not recognized as an internal or external command`. If a task requires running tests,
  first ensure a C++17-capable g++ is installed and on PATH; there is no PlatformIO-bundled native
  compiler to fall back on. Do not attempt to work around this by building for an ESP32 env
  instead — the `native` tests are host-side logic tests, not hardware builds.
- Test files include `../support/...` relative includes — do not move test files without updating
  those includes.

## Lint / Static Analysis
There is no dedicated lint config (no `.clang-format`, `.clang-tidy`, ESLint, etc.) in this repo.
Code style is whatever `platformio run` accepts via the Arduino/gcc compiler warnings. Treat any
new compiler warnings introduced by a change as something to fix before finishing.

## CI / Validation Pipelines
There is currently **no GitHub Actions workflow directory** (`.github/workflows/`) present in this
repo, despite README/projectsummary mentioning a legacy `issue-commenter.yml` — it does not exist
in the current tree, so there is no automated CI build to satisfy. The most reliable
pre-submission validation is manually running the `platformio run -e T3_V1_6_SX1276` build command
above and, when compiler tooling is available, `platformio test -e native`.

## Project Layout Notes
- `platformio.ini` — single source of truth for build envs, board configs (`boards_dir = boards`),
  and dependencies (DallasTemperature, INA226_WE, U8g2, OneWire pinned to 2.3.7, RadioLib,
  PubSubClient, ArduinoJson). Edit here to add/change library dependencies or build flags.
- `modes/Sensor/main.cpp`, `modes/GateWay/main.cpp` — the two firmware entry points; functions are
  annotated inline indicating whether they run in Operation mode, Dev mode, or both — preserve
  this commenting convention when editing.
- `modes/GateWay/secrets.h`, `elog_cfg.h` — gateway-specific config/logging headers.
- `payloads.h` (in each mode dir) — binary wire-format structs for LoRa telemetry/config packets;
  changes here affect both Sensor and GateWay and must stay wire-compatible with the format
  documented in README.md.
- `lib/LoraHomeCommon/` — shared first-party library code (not vendored, safe to edit).
- `lib/*` (all other subfolders) — vendored third-party Arduino libraries; excluded from AI
  indexing via `.aiexclude`; avoid editing.
- `test/support/LoraHomeProtocolModel.h` — test-only mirror/helpers for payload logic.
- `AGENTS.md` — repo-specific working rules (keep README/projectsummary.md updated on logic
  changes, don't touch files under `.aiexclude`, preserve existing comments/commented-out code,
  report context size, only touch README for logic changes not syntax fixes).
- `.github/projectsummary.md` — authoritative concise architecture summary; prefer reading this
  over re-deriving architecture from source when possible, to save context/exploration.

## Practical Guidance for Making Changes
1. Read `.github/projectsummary.md` and the relevant `modes/<Mode>/main.cpp` before editing.
2. Make the code change, keeping inline Operation/Dev-mode comments intact.
3. Validate with the `platformio run -e T3_V1_6_SX1276` command above (fix `intelhex` first if
   this is a fresh environment).
4. If the change affects payload structs or protocol logic, also run/update the `native` unit
   tests when a compiler is available, and update `README.md` + `.github/projectsummary.md`
   describing the behavior change (skip README updates for pure syntax fixes, per `AGENTS.md`).
5. Do not modify vendored libraries under `lib/` (except `lib/LoraHomeCommon/`) unless explicitly
   required.
