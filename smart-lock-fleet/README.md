# Smart Lock Fleet — Custom Metrics & Dashboards

> Companion code for the Spotflow blog post:
> **[Custom Metrics & Dashboards for Embedded Devices: From Sensor Data to Product Analytics](https://spotflow.io/blog/custom-metrics-dashboards-embedded-devices)**

This example demonstrates how to instrument a fleet of connected smart locks with **custom application metrics** using Spotflow, and how to build a **custom dashboard** in the Spotflow web app that goes beyond built-in system health into product analytics.

The smart lock is a realistic embedded use case: an ARM Cortex-M33 MCU with Bluetooth, NFC, and keypad input, running Zephyr RTOS and connected to the cloud via MQTT over TLS. The firmware reports metrics that answer both engineering questions (is the device healthy?) and product questions (how are users interacting with it?).

## What this example demonstrates

- **Custom float metrics with labels** — `lock_operation_duration_ms` broken down by `operation` (lock/unlock) and `method` (NFC, keypad, Bluetooth)
- **Custom int metrics and event reporting** — `door_opened` and `auth_failure`, each a discrete countable event
- **Aggregation intervals** — 1-minute aggregation for duration metrics vs. no aggregation for event metrics
- **Per-thread metric ownership** — `battery_level_percent` registered and reported from a dedicated `K_THREAD_DEFINE` battery monitor thread
- **System metrics alongside custom metrics** — `CONFIG_SPOTFLOW_METRICS_SYSTEM=y` for automatic CPU, heap, and stack telemetry
- **Custom Dashboard** — how to build a product analytics view in the Spotflow web app combining all of the above

## Hardware

Any Zephyr-supported board with network connectivity (Wi-Fi or Ethernet). Tested on:

- [NXP FRDM-RW612](https://www.nxp.com/design/microcontrollers/arm-cortex-m/rw6xx-rtos-ready-wireless-mcus:FRDM-RW612) (Wi-Fi 6)
- [Nordic nRF7002DK](https://www.nordicsemi.com/Products/Development-hardware/nRF7002-DK) (Wi-Fi)

For other boards, refer to the [board overlays in the Spotflow Device SDK](https://github.com/spotflow-io/device-sdk/tree/main/zephyr/samples/logs/boards) for configuration inspiration.

## Prerequisites

- Zephyr SDK and `west` installed — see the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- A Spotflow account and ingest key — [sign up for free](https://app.spotflow.io/signup), then manage your keys at [app.spotflow.io/ingest-keys](https://app.spotflow.io/ingest-keys)

## Setup

### Step 1: Create the west workspace

Run the Spotflow setup script to create a west workspace that includes the Spotflow module and this example:

**Linux / macOS:**
```sh
source <(curl --proto '=https' --tlsv1.2 -sSf https://downloads.spotflow.io/spotflowup.sh) --zephyr --board <your-board>
```

**Windows (PowerShell):**
```powershell
Invoke-Expression "& {$(Invoke-RestMethod -Uri 'https://downloads.spotflow.io/spotflowup.ps1' -UseBasicParsing) } -zephyr -board <your-board>"
```

Replace `<your-board>` with your Zephyr board identifier (e.g. `frdm_rw612`, `nrf7002dk_nrf5340_cpuapp`).

Alternatively, set up the workspace manually following the [Spotflow Device SDK workspace setup instructions](https://github.com/spotflow-io/device-sdk/blob/main/zephyr/ci/README.md#workspace-setup-scripts).

### Step 2: Clone this example into the workspace

Place the contents of this folder (or clone this repository) somewhere accessible within your west workspace, for example:

```
<workspace>/
└── firmware-observability-examples/
    └── smart-lock-fleet/
```

### Step 3: Configure the application

Open `prj.conf` and fill in your settings:

**Wi-Fi:**
```
CONFIG_NET_WIFI_SSID="<Your Wi-Fi SSID>"
CONFIG_NET_WIFI_PASSWORD="<Your Wi-Fi password>"

CONFIG_SPOTFLOW_DEVICE_ID="smart-lock-001"
CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"
```

**Ethernet:**
```
CONFIG_SPOTFLOW_USE_ETH=y

CONFIG_SPOTFLOW_DEVICE_ID="smart-lock-001"
CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"
```

### Step 4: Build and flash

```sh
cd firmware-observability-examples/smart-lock-fleet
west build --pristine --board <your-board>
west flash
```

Once the device is running and shows `MQTT connected!` on UART, it is streaming metrics to Spotflow. Open [app.spotflow.io/devices](https://app.spotflow.io/devices) to see your device appear.

### Step 5: Build the custom dashboard

In the Spotflow web app, navigate to **Dashboards → + Create Dashboard** and add widgets for:

- `lock_operation_duration_ms` grouped by `method` (line chart, milliseconds)
- `door_opened` (bar chart, count)
- `auth_failure` grouped by `method` (bar chart, count)
- `battery_level_percent` (line chart, percent)

See [Create Custom Dashboard](https://docs.spotflow.io/guides/custom-dashboards) for the step-by-step guide.

## Project structure

```
smart-lock-fleet/
├── CMakeLists.txt      — Zephyr application build configuration
├── prj.conf            — Kconfig: Spotflow, Wi-Fi/Ethernet, metrics
├── west.yml            — West workspace manifest (references Spotflow Device SDK)
└── src/
    ├── main.c          — Application entry point; initializes metrics and starts threads
    ├── lock.c          — Lock/unlock simulation; reports operation duration, door open,
    │                     and authentication failure metrics
    ├── lock.h          — Public API for lock module
    ├── battery.c       — Battery monitor thread; reports battery_level_percent metric
    ├── battery.h       — Public API for battery module
    └── net.h           — Network helper: Wi-Fi / Ethernet connection setup
```

## Related links

- [Custom Metrics & Dashboards for Embedded Devices](https://spotflow.io/blog/custom-metrics-dashboards-embedded-devices) — the blog post this example accompanies
- [Fundamentals: Metrics](https://docs.spotflow.io/fundamentals/metrics) — aggregation intervals, system vs. custom metrics, transport protocol
- [Fundamentals: Dashboards](https://docs.spotflow.io/fundamentals/dashboards) — device dashboard, overview dashboard, custom dashboards
- [Guide: Metrics with Zephyr](https://docs.spotflow.io/guides/zephyr/metrics-zephyr) — full Zephyr integration reference
- [Guide: Create Custom Dashboard](https://docs.spotflow.io/guides/custom-dashboards) — step-by-step dashboard builder walkthrough
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk) — west module source and basic samples
