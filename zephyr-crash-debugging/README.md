# Zephyr RTOS Crash Debugging with Spotflow

> Companion code for the Spotflow blog post:
> **[Debugging a Zephyr RTOS Crash with Spotflow AI Analysis](https://spotflow.io/blog/zephyr-crash-debugging)**

This example shows how to instrument a Zephyr RTOS application so that when a crash occurs in the field, [Spotflow](https://docs.spotflow.io/) automatically collects the core dump, and the built-in AI analysis identifies the root cause, eliminating the need for a serial cable.

Tested on: **NXP FRDM-RW612**. The patterns apply to any Zephyr board supported by the Spotflow device module.

## What this example demonstrates

- Zephyr RTOS core dump collection via `CONFIG_SPOTFLOW_COREDUMPS=y` (three Kconfig lines)
- Automatic crash upload on the next boot, over MQTT/TLS
- Remote logs (`LOG_WRN`, `LOG_ERR`) showing the warning pattern before the crash
- Application metrics: `temperature_celsius` and `read_errors`
- System metrics collected automatically: `boot_reset`, heap, CPU, connection state
- **Spotflow AI Crash Analysis**: stack traces + register values + plain-language root cause explanation
- A deterministic, button-triggered crash path for reproducible demos

## Crash scenario

The application simulates a temperature monitoring node. An alert callback (`g_alert_callback`) is never registered on this device variant, it was provisioned as a sensor-only unit. The firmware calls the callback unconditionally when temperature exceeds the threshold, without checking for `NULL`. This is a realistic pattern: conditionally registered callbacks called from a shared processing path.

Crash path:

1. Press the user button (SW2)
2. Threshold is lowered to 20 °C, below the simulated 28–34 °C range
3. After 3 loop iterations, `check_threshold()` calls `g_alert_callback(temp)` → `NULL` function pointer → fatal fault
4. Zephyr saves the core dump to the `coredump-partition` in flash
5. After reboot, Spotflow uploads the core dump and AI analysis is available within seconds

## Hardware

[NXP FRDM-RW612](https://docs.zephyrproject.org/latest/boards/nxp/frdm_rw612/doc/index.html): ARM Cortex-M33, 260 MHz, 1.2 MB SRAM, Wi-Fi 6, built-in QSPI flash.

## Prerequisites

- A Spotflow account and [ingest key](https://docs.spotflow.io/fundamentals/device-authorization): [sign up for free](https://app.spotflow.io/signup)
- Git
- Python 3.12+

## Setup

### Step 1: Create the workspace environment

```powershell
python -m venv .venv
# Windows (PowerShell):
.venv\Scripts\Activate.ps1
# Linux / macOS:
# source .venv/bin/activate

pip install west
```

### Step 2: Fetch the pinned dependencies

`.west/config` is already committed and points to `west.yml`, so no `west init` step is needed.

> **Windows note:** Use a short path such as `C:\ws\firmware-observability-examples\zephyr-crash-debugging` to avoid path-length issues during `west update`.

```sh
west update --fetch-opt=--depth=1 --narrow
west packages pip --install
```

The manifest is pinned to:

- Zephyr `v4.4.0`
- Spotflow Device SDK `main`

### Step 3: Install the Zephyr SDK toolchain

```sh
west sdk install --version 1.0.1 --toolchains arm-zephyr-eabi
```

### Step 4: Configure the application

Copy `credentials-sample.conf` to `credentials.conf` and fill in your values:

```powershell
Copy-Item credentials-sample.conf credentials.conf
```

Then edit `credentials.conf`:

```ini
CONFIG_NET_WIFI_SSID="<your-ssid>"
CONFIG_NET_WIFI_PASSWORD="<your-password>"
CONFIG_SPOTFLOW_DEVICE_ID="zephyr-crash-debugging-001"
CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"
```

## Build and flash

```sh
west build --pristine --board frdm_rw612
west flash
```

## What to expect on UART

On a healthy boot:

```
[00:00:00.xxx] <inf> main: Zephyr crash debugging example starting
[00:00:00.xxx] <inf> main: This demo shows how Spotflow AI Crash Analysis identifies the root cause of a Zephyr RTOS crash.
[00:00:03.xxx] <inf> spotflow_sample_wifi: Connecting to SSID: <your-ssid>
[00:00:05.xxx] <inf> spotflow_net: MQTT connected!
[00:00:05.xxx] <inf> sensor_node: Sensor node ready. Press the user button to reproduce the crash.
[00:00:07.xxx] <inf> sensor_node: Sensor reading: 31.4 C
[00:00:09.xxx] <inf> sensor_node: Sensor reading: 29.7 C
...
```

## Crash reproduction

Press the user button (**SW2**) on the FRDM-RW612.

You should then see:

```
[...] <wrn> main: User button pressed. Arming reproducible crash path.
[...] <wrn> sensor_node: Crash path armed. Lowering temperature threshold to 20.0 C.
[...] <inf> sensor_node: Sensor reading: 31.4 C
[...] <wrn> sensor_node: Temperature threshold exceeded: 31.4 C (threshold: 20.0 C)
[...] <inf> sensor_node: Sensor reading: 30.1 C
[...] <wrn> sensor_node: Temperature threshold exceeded: 30.1 C (threshold: 20.0 C)
[...] <inf> sensor_node: Sensor reading: 32.8 C
[...] <wrn> sensor_node: Temperature threshold exceeded: 32.8 C (threshold: 20.0 C)
*** Zephyr Fatal Error ***
```

After reboot, the device reconnects and Spotflow uploads the core dump. The crash report with AI analysis is available in the [Events](https://app.spotflow.io/) page within seconds.

## Upload ELF for AI symbol resolution

To unlock full symbol names and variable values in the AI analysis, upload the ELF file with debug symbols to Spotflow:

```sh
west spotflow upload-elf build/zephyr/zephyr.elf
```

See [Firmware Management](https://docs.spotflow.io/fundamentals/firmware-management) for details.

## Metrics

Custom application metrics registered by this example:

- `temperature_celsius`: raw temperature reading every 2 s
- `read_errors`: event fired on simulated I2C read timeout

System metrics collected automatically (no application code required):

- `boot_reset`: reset cause on every boot
- `heap_free_bytes`, `cpu_utilization_percent`, `connection_mqtt_connected`, and others

## Project structure

- `src/main.c`: entry point: button setup, network init, main sensor loop
- `src/sensor_node.c`: metric registration, sensor simulation, and the crash path
- `boards/frdm_rw612.conf`: board-specific Kconfig fragment
- `boards/frdm_rw612.overlay`: coredump flash partition (2 MiB at end of W25Q512JV)
- `src/net/`: Wi-Fi / Ethernet connectivity, shared from the observability examples collection
- `west.yml`: pinned dependency manifest (Zephyr v4.4.0, Spotflow Device SDK)
- `credentials-sample.conf`: template for Wi-Fi and Spotflow credentials

## Related links

- [Blog post: Debugging a Zephyr RTOS Crash with Spotflow AI Analysis](https://spotflow.io/blog/zephyr-crash-debugging)
- [Fundamentals: Crash reports & core dumps](https://docs.spotflow.io/fundamentals/crash-reports)
- [Guide: Crash reports with Zephyr](https://docs.spotflow.io/guides/zephyr/crash-reports-zephyr)
- [Fundamentals: Metrics](https://docs.spotflow.io/fundamentals/metrics)
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk)
