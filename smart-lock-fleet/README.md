# Smart Lock Fleet — Custom Metrics & Dashboards

> Companion code for the Spotflow blog post:
> **[Custom Metrics & Dashboards for Embedded Devices: From Sensor Data to Product Analytics](https://spotflow.io/blog/custom-metrics-dashboards-embedded-devices?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_smart_lock_readme&utm_content=link_blog_post)**

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
- [Espressif ESP32-C3-DevKitC-02](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c3/esp32-c3-devkitc-02/index.html) (Wi-Fi)

For other boards, refer to the [board overlays in the Spotflow Device SDK](https://github.com/spotflow-io/device-sdk/tree/main/zephyr/samples/logs/boards) for configuration inspiration.

## Prerequisites

**Both setups require:**
- A Spotflow account and ingest key — [sign up for free](https://app.spotflow.io/signup?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_smart_lock_readme&utm_content=cta_signup), then manage your keys at [app.spotflow.io/ingest-keys](https://app.spotflow.io/ingest-keys?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_smart_lock_readme&utm_content=link_ingest_keys)
- Git

**Zephyr (non-Nordic boards) additionally requires:**
- Python 3.12+
- Flashing software matching your board (e.g. J-Link or OpenOCD)

**nRF Connect SDK (Nordic nRF7002DK) additionally requires:**
- [SEGGER J-Link](https://www.segger.com/downloads/jlink/)
- `nrfutil` — see the [nRF Connect SDK installation guide](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/installation.html)

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

```sh
# 1. Clone this repository
git clone https://github.com/spotflow-io/firmware-observability-examples
cd firmware-observability-examples/smart-lock-fleet

# 2. Create and activate a virtual environment
python -m venv .venv
# Linux/macOS:
source .venv/bin/activate
# Windows (PowerShell):
# .venv/Scripts/Activate.ps1

pip install west

# 3. Fetch all dependencies
# .west/config is already committed in the repo and points to west.yml,
# so no "west init" is needed — west update reads it directly.
west update --fetch-opt=--depth=1 --narrow
west packages pip --install

# 4. Install the Zephyr SDK toolchain(s) for your target board(s):
# ARM boards (frdm_rw612, cy8cproto_062_4343w, rpi_pico/rp2040/w):
west sdk install --version 0.17.4 --toolchains arm-zephyr-eabi
# RISC-V boards (esp32c3_devkitc, esp32c3_devkitm, esp32c6_devkitc/esp32c6/hpcore):
west sdk install --version 0.17.4 --toolchains riscv64-zephyr-elf
# Xtensa boards (esp32s3_devkitc/esp32s3/procpu):
west sdk install --version 0.17.4 --toolchains xtensa-espressif_esp32s3_zephyr-elf

# 5. Download required binary blobs (board-dependent)
# NXP FRDM-RW612: signed Wi-Fi/BLE firmware blobs
west blobs fetch hal_nxp --auto-accept
# ESP32 boards (esp32c3_devkitc, esp32c3_devkitm, esp32c6_devkitc/esp32c6/hpcore, esp32s3_devkitc/esp32s3/procpu):
west blobs fetch hal_espressif --auto-accept
# Infineon board and Raspberry Pi Pico W (cy8cproto_062_4343w, rpi_pico/rp2040/w): Wi-Fi firmware blobs for the Infineon CYW43439
west blobs fetch hal_infineon --auto-accept
```

---

#### nRF Connect SDK — Nordic nRF7002DK

The nRF7002DK requires the **nRF Connect SDK** (NCS) instead of vanilla Zephyr. NCS ships its own
pinned Zephyr fork, so a separate manifest (`west-nrf.yml`) is provided.

> **Windows note:** NCS builds generate very deep object file paths. On Windows, clone into a short directory to stay under the 260-character path limit, some build tools enforce this limit even when long paths are enabled in the registry:
> ```powershell
> git clone https://github.com/spotflow-io/firmware-observability-examples C:\nws
> Rename-Item C:\nws\smart-lock-fleet app
> cd C:\nws\app
> ```

```sh
# 1. Clone this repository
git clone https://github.com/spotflow-io/firmware-observability-examples
cd firmware-observability-examples/smart-lock-fleet

# 2. Switch the manifest to the NCS manifest
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

# 3. Install the NCS toolchain
nrfutil install sdk-manager
nrfutil sdk-manager toolchain install --ncs-version v3.2.4

# 4. Fetch all dependencies
# .west/config is already committed in the repo, so no "west init" is needed.
nrfutil sdk-manager toolchain launch --ncs-version v3.2.4 -- west update --fetch-opt=--depth=1 --narrow
```

> **Note:** `west sdk install` (Zephyr SDK) is **not** used with NCS.
> The NCS toolchain installed by `nrfutil` already includes a pinned GCC for the nRF5340.

---

### Step 2: Configure the application

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

### Step 3: Build and flash

**Board targets:**

| Board | Manifest | Target |
|---|---|---|
| NXP FRDM-RW612 | `west.yml` | `frdm_rw612` |
| Nordic nRF7002DK | `west-nrf.yml` | `nrf7002dk/nrf5340/cpuapp/ns` |
| Espressif ESP32-C3 DevKitC-02 | `west.yml` | `esp32c3_devkitc` |
| Espressif ESP32-C3 DevKitM | `west.yml` | `esp32c3_devkitm` |
| Espressif ESP32-C6 DevKitC | `west.yml` | `esp32c6_devkitc/esp32c6/hpcore` |
| Espressif ESP32-S3 DevKitC | `west.yml` | `esp32s3_devkitc/esp32s3/procpu` |
| Infineon CY8CPROTO-062-4343W | `west.yml` | `cy8cproto_062_4343w` |
| Raspberry Pi Pico W | `west.yml` | `rpi_pico/rp2040/w` |

---

#### Zephyr — non-Nordic boards

```sh
west build --pristine --board <your-board-target>
west flash
```

---

#### nRF Connect SDK — Nordic nRF7002DK

The `west build` and `west flash` commands must be run inside the **nRF Connect toolchain
environment**. Use `nrfutil sdk-manager toolchain launch` to wrap each command:

```sh
nrfutil sdk-manager toolchain launch --ncs-version v3.2.4 -- west build --pristine --board nrf7002dk/nrf5340/cpuapp/ns
nrfutil sdk-manager toolchain launch --ncs-version v3.2.4 -- west flash
```

Alternatively, use the integrated terminal in
[nRF Connect for VS Code](https://docs.nordicsemi.com/bundle/nrf-connect-vscode/page/guides/extension_nrfconnect_profile.html)
or run `nrfutil sdk-manager toolchain launch --help` for full options.

Once the device is running and shows `MQTT connected!` on UART, it is streaming metrics to Spotflow. Open [app.spotflow.io/devices](https://app.spotflow.io/devices?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_smart_lock_readme&utm_content=link_devices) to see your device appear.

### Step 4: Build the custom dashboard

In the Spotflow web app, navigate to **Dashboards → + Create Dashboard** and add widgets for:

- `lock_operation_duration_ms` grouped by `method` (line chart, milliseconds)
- `door_opened` (bar chart, count)
- `auth_failure` grouped by `method` (bar chart, count)
- `battery_level_percent` (line chart, percent)

See [Create Custom Dashboard](https://docs.spotflow.io/guides/custom-dashboards?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_smart_lock_readme&utm_content=link_guide_custom_dashboards) for the step-by-step guide.

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
    └── net.h                — Network helper: Wi-Fi / Ethernet connection setup
```

## Related links

- [Custom Metrics & Dashboards for Embedded Devices](https://spotflow.io/blog/custom-metrics-dashboards-embedded-devices?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_smart_lock_readme&utm_content=link_blog_post) — the blog post this example accompanies
- [Fundamentals: Metrics](https://docs.spotflow.io/fundamentals/metrics?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_smart_lock_readme&utm_content=link_fundamentals_metrics) — aggregation intervals, system vs. custom metrics, transport protocol
- [Fundamentals: Dashboards](https://docs.spotflow.io/fundamentals/dashboards?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_smart_lock_readme&utm_content=link_fundamentals_dashboards) — device dashboard, overview dashboard, custom dashboards
- [Guide: Metrics with Zephyr](https://docs.spotflow.io/guides/zephyr/metrics-zephyr?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_smart_lock_readme&utm_content=link_guide_metrics_zephyr) — full Zephyr integration reference
- [Guide: Create Custom Dashboard](https://docs.spotflow.io/guides/custom-dashboards?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_smart_lock_readme&utm_content=link_guide_custom_dashboards) — step-by-step dashboard builder walkthrough
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk) — west module source and basic samples
