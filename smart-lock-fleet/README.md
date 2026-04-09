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

- A Spotflow account and ingest key — [sign up for free](https://app.spotflow.io/signup), then manage your keys at [app.spotflow.io/ingest-keys](https://app.spotflow.io/ingest-keys)
- `west` and Python 3 installed
- A toolchain matching your target platform (see per-platform instructions below):
  - **Zephyr / ESP32 boards** — Zephyr SDK; see the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
  - **Nordic nRF7002DK** — `nrfutil` (nRF Connect SDK toolchain manager); see the [nRF Connect SDK installation guide](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/installation.html)

## Setup

### Step 1: Create the west workspace

This project ships two west manifests:

| Manifest file | Use for |
|---|---|
| `west.yml` (default) | All non-Nordic boards: NXP, ESP32, Infineon, RPi Pico W |
| `west-nrf.yml` | Nordic nRF7002DK (nRF Connect SDK) |

Pick the sub-section that matches your board.

---

#### Zephyr — non-Nordic boards (NXP FRDM-RW612, ESP32, Infineon, RPi Pico W)

**Using the Spotflow setup script (easiest):**

Linux / macOS:
```sh
source <(curl --proto '=https' --tlsv1.2 -sSf https://downloads.spotflow.io/spotflowup.sh) --zephyr --board <your-board>
```

Windows (PowerShell):
```powershell
Invoke-Expression "& {$(Invoke-RestMethod -Uri 'https://downloads.spotflow.io/spotflowup.ps1' -UseBasicParsing) } -zephyr -board <your-board>"
```

Replace `<your-board>` with your Zephyr board identifier (e.g. `frdm_rw612`, `esp32c3_devkitm`).

**Manual setup:**

```sh
# 1. Create a workspace directory and activate a virtual environment
mkdir spotflow-ws && cd spotflow-ws
python -m venv .venv
# Linux/macOS:
source .venv/bin/activate
# Windows (PowerShell):
# .venv/Scripts/Activate.ps1

pip install west

# 2. Clone this repository into the workspace
git clone https://github.com/spotflow-io/firmware-observability-examples
cd firmware-observability-examples/smart-lock-fleet

# 3. Initialize the west workspace using the default Zephyr manifest
west init -l .
west update --fetch-opt=--depth=1 --narrow
west packages pip --install

# 4. Install the Zephyr SDK (adjust toolchain for your board's architecture)
west sdk install --version 0.17.4 --toolchains arm-zephyr-eabi

# 5. ESP32 boards only: download the required binary blobs
# (needed for esp32c3_devkitm, esp32c6_devkitc_esp32c6_hpcore, esp32s3_devkitc_esp32s3_procpu)
west blobs fetch hal_espressif --auto-accept
```

> The `.west/config` in this project already points to `west.yml` by default, so `west init -l .` picks it up automatically.

---

#### nRF Connect SDK — Nordic nRF7002DK

The nRF7002DK requires the **nRF Connect SDK** (NCS) instead of vanilla Zephyr. NCS ships its own
pinned Zephyr fork, so a separate manifest (`west-nrf.yml`) is provided.

**Manual setup:**

```sh
# 1. Create a workspace directory and activate a virtual environment
mkdir spotflow-nrf-ws && cd spotflow-nrf-ws
python -m venv .venv
# Linux/macOS:
source .venv/bin/activate
# Windows (PowerShell):
# .venv/Scripts/Activate.ps1

pip install west

# 2. Clone this repository into the workspace
git clone https://github.com/spotflow-io/firmware-observability-examples
cd firmware-observability-examples/smart-lock-fleet

# 3. Switch the manifest to the NCS manifest before initializing
#    Edit .west/config and change:
#      file = west.yml  →  file = west-nrf.yml
#    Also remove the [zephyr] section (NCS provides Zephyr through its own import):
#      [zephyr]
#      base = external/zephyr
#
#    The resulting .west/config should look like:
#
#      [manifest]
#      path = .
#      file = west-nrf.yml

# 4. Initialize and update the workspace
west init -l .
west update --fetch-opt=--depth=1 --narrow
west packages pip --install

# 5. Install the NCS toolchain (replaces "west sdk install" for NCS boards)
nrfutil sdk-manager toolchain install --ncs-version v3.2.4
```

> **Note:** `west sdk install` (Zephyr SDK) is **not** used with NCS.
> The NCS toolchain installed by `nrfutil` already includes a pinned GCC for the nRF5340.

---

### Step 2: Clone this example into the workspace

If you followed the manual setup above, the repository is already in place. If you used the
Spotflow setup script, place the contents of this folder somewhere accessible within your west
workspace:

```
<workspace>/
└── firmware-observability-examples/
    └── smart-lock-fleet/
```

### Step 3: Configure the application

Copy `credentials-sample.conf` to `credentials.conf` and fill in your values (this file is
excluded from version control by `.gitignore`):

```sh
cp credentials-sample.conf credentials.conf
```

Then edit `credentials.conf`:

**Wi-Fi boards:**
```
CONFIG_NET_WIFI_SSID="<Your Wi-Fi SSID>"
CONFIG_NET_WIFI_PASSWORD="<Your Wi-Fi password>"
CONFIG_SPOTFLOW_DEVICE_ID="smart-lock-001"
CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"
```

**Ethernet boards:**
```
CONFIG_SPOTFLOW_USE_ETH=y
CONFIG_SPOTFLOW_DEVICE_ID="smart-lock-001"
CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"
```

Alternatively, set the values directly in `prj.conf`.

### Step 4: Build and flash

Navigate to the `smart-lock-fleet` directory and run `west build` with your board target:

```sh
cd firmware-observability-examples/smart-lock-fleet
west build --pristine --board <your-board>
west flash
```

**Board targets:**

| Board | Target |
|---|---|
| NXP FRDM-RW612 | `frdm_rw612` |
| Nordic nRF7002DK (Zephyr) | `nrf7002dk_nrf5340_cpuapp` |
| Nordic nRF7002DK (NCS, TF-M) | `nrf7002dk_nrf5340_cpuapp_ns` |
| Espressif ESP32-C3 DevKitM | `esp32c3_devkitm` |
| Espressif ESP32-C6 DevKitC | `esp32c6_devkitc_esp32c6_hpcore` |
| Espressif ESP32-S3 DevKitC | `esp32s3_devkitc_esp32s3_procpu` |
| Infineon CY8CPROTO-062-4343W | `cy8cproto_062_4343w` |
| Raspberry Pi Pico W | `rpi_pico_rp2040_w` |

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
├── CMakeLists.txt           — Zephyr application build configuration
├── prj.conf                 — Kconfig: Spotflow, Wi-Fi/Ethernet, metrics
├── credentials-sample.conf  — Template for credentials (copy to credentials.conf)
├── west.yml                 — West manifest for Zephyr (non-Nordic boards)
├── west-nrf.yml             — West manifest for nRF Connect SDK (Nordic nRF7002DK)
├── .west/config             — West workspace config; change manifest.file to
│                              west-nrf.yml when building for Nordic boards
├── boards/                  — Board-specific Kconfig fragments and DT overlays
└── src/
    ├── main.c               — Application entry point; initializes metrics and starts threads
    ├── lock.c               — Lock/unlock simulation; reports operation duration, door open,
    │                          and authentication failure metrics
    ├── lock.h               — Public API for lock module
    ├── battery.c            — Battery monitor thread; reports battery_level_percent metric
    ├── battery.h            — Public API for battery module
    └── net.h                — Network helper: Wi-Fi / Ethernet connection setup
```

## Related links

- [Custom Metrics & Dashboards for Embedded Devices](https://spotflow.io/blog/custom-metrics-dashboards-embedded-devices) — the blog post this example accompanies
- [Fundamentals: Metrics](https://docs.spotflow.io/fundamentals/metrics) — aggregation intervals, system vs. custom metrics, transport protocol
- [Fundamentals: Dashboards](https://docs.spotflow.io/fundamentals/dashboards) — device dashboard, overview dashboard, custom dashboards
- [Guide: Metrics with Zephyr](https://docs.spotflow.io/guides/zephyr/metrics-zephyr) — full Zephyr integration reference
- [Guide: Create Custom Dashboard](https://docs.spotflow.io/guides/custom-dashboards) — step-by-step dashboard builder walkthrough
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk) — west module source and basic samples
