# ESP32 Industrial Sensor — Spotflow Observability Example

> Companion code for the Spotflow blog post:
> **[ESP32 Crash Debugging That Actually Works: Logs, Metrics & AI Analysis with Zephyr](https://feature-blog-post-esp32-zeph.platform-website-9gk.pages.dev/blog/esp32-crash-debugging-zephyr/)**

This example demonstrates all three observability pillars of the [Spotflow](https://spotflow.io) platform working together on an ESP32-based industrial sensor node running Zephyr RTOS:

- **Structured remote logging** — every `LOG_*` call is forwarded to Spotflow over MQTT in real time
- **Metrics with alerting** — sensor readings, error counters, and system health metrics with Spotflow alert rules
- **AI-assisted crash analysis** — Zephyr coredumps written to flash on panic, uploaded automatically on the next boot, and analyzed by Spotflow's AI agent

The example simulates a real-world bug: a use-after-free on the sensor handle that causes a NULL dereference in the sensor polling thread. Press the **BOOT button** on the board to trigger the crash on demand. When the device crashes, Spotflow's AI agent reconstructs the call stack and explains the root cause without requiring JTAG or a serial cable.

## What this example demonstrates

- **Remote structured logging** — `LOG_MODULE_REGISTER` + standard Zephyr `LOG_INF/WRN/ERR` macros forwarded automatically to the Spotflow cloud log stream
- **Custom float metrics** — `sensor_temperature_celsius` and `sensor_humidity_percent` reported as immediate, unaggregated readings
- **Custom int metrics for alerting** — `sensor_read_errors` and `sensor_crashes` aggregated per minute; used as alert sources in the Spotflow dashboard
- **System metrics** — heap, CPU, thread stacks, network I/O, and reset cause via `CONFIG_SPOTFLOW_METRICS_SYSTEM=y`; uptime heartbeat via the separate `CONFIG_SPOTFLOW_METRICS_HEARTBEAT=y`
- **Coredump collection** — flash partition configured per board; Spotflow reads and uploads the dump on the next boot
- **AI crash analysis** — Spotflow's AI agent receives the coredump, reconstructs the call stack, and produces a root cause analysis with a suggested fix

## Hardware

Any of the following ESP32 boards connected to a Wi-Fi network:

| Board | Zephyr target |
|---|---|
| [Espressif ESP32-C3 DevKitC](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c3/esp32-c3-devkitc-02/index.html) | `esp32c3_devkitc` |
| [Espressif ESP32-C6 DevKitC](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c6/esp32-c6-devkitc-1/index.html) | `esp32c6_devkitc/esp32c6/hpcore` |
| [Espressif ESP32-S3 DevKitC](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/index.html) | `esp32s3_devkitc/esp32s3/procpu` |

For other boards, refer to the [board overlays in the Spotflow Device SDK](https://github.com/spotflow-io/device-sdk/tree/main/zephyr/samples/coredumps/boards) for configuration inspiration.

## Prerequisites

**Required for all setups:**
- A Spotflow account and ingest key — [sign up for free](https://app.spotflow.io/signup?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_sensor_readme&utm_content=cta_signup), then manage your keys at [app.spotflow.io/ingest-keys](https://app.spotflow.io/ingest-keys?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_sensor_readme&utm_content=link_ingest_keys)
- Git
- Python 3.12+
- Flashing software for your board (e.g. `esptool` for ESP32 boards, installed automatically as part of `west packages pip --install`)

## Setup

### Step 1: Create the west workspace

```sh
# 1. Clone this repository
git clone https://github.com/spotflow-io/firmware-observability-examples
cd firmware-observability-examples/esp32-industrial-sensor

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

# 4. Install the Zephyr SDK toolchain for your target board
# ESP32-C3 and ESP32-C6 (RISC-V):
west sdk install --version 1.0.1 --toolchains riscv64-zephyr-elf
# ESP32-S3 (Xtensa):
west sdk install --version 1.0.1 --toolchains xtensa-espressif_esp32s3_zephyr-elf

# 5. Download the ESP32 binary blobs (Wi-Fi firmware)
west blobs fetch hal_espressif --auto-accept
```

---

### Step 2: Configure the application

Copy `credentials-sample.conf` to `credentials.conf` and fill in your values (this file is
excluded from version control by `.gitignore`):

```sh
cp credentials-sample.conf credentials.conf
```

Then edit `credentials.conf`:

```
CONFIG_NET_WIFI_SSID="<Your Wi-Fi SSID>"
CONFIG_NET_WIFI_PASSWORD="<Your Wi-Fi password>"
CONFIG_SPOTFLOW_DEVICE_ID="<your-device-id>"
CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"
```

---

### Step 3: Build and flash

**Board targets:**

| Board | Target |
|---|---|
| Espressif ESP32-C3 DevKitC | `esp32c3_devkitc` |
| Espressif ESP32-C6 DevKitC | `esp32c6_devkitc/esp32c6/hpcore` |
| Espressif ESP32-S3 DevKitC | `esp32s3_devkitc/esp32s3/procpu` |

```sh
west build --pristine --board <your-board-target>
west flash
```

For example, for the ESP32-C3 DevKitC:

```sh
west build --pristine --board esp32c3_devkitc
west flash
```

---

### Step 4: View results in Spotflow

Once the device is running and shows `Connected to <SSID>` on UART, it is streaming logs and metrics to Spotflow. Open [app.spotflow.io/devices](https://app.spotflow.io/devices?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_sensor_readme&utm_content=link_devices) to see your device appear.

Press the **BOOT button** on the board to trigger the simulated crash. The firmware will deinit the sensor handle on the next completed read and set the pointer to NULL; five seconds later `sensor_read()` dereferences NULL and the kernel panics. On the next boot you will see:

- The **uptime heartbeat alert** triggered — no `uptime_ms` received for more than 5 minutes
- The `boot_reset` system metric showing `reason=panic`
- The full log sequence before the crash in the **Logs** view
- The AI-generated call stack reconstruction and fix suggestion in the **Crash Reports** view

See [Set Up Alerts](https://docs.spotflow.io/guides/set-up-alerts?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_sensor_readme&utm_content=link_guide_alerts) for how to configure the uptime heartbeat and error-rate alert rules shown in the blog post.

## Project structure

```
esp32-industrial-sensor/
├── CMakeLists.txt           — Zephyr application build configuration
├── Kconfig                  — Sources src/net/Kconfig
├── prj.conf                 — Kconfig: Spotflow logs + metrics + coredumps + networking
├── credentials-sample.conf  — Template for credentials (copy to credentials.conf)
├── west.yml                 — West manifest: Zephyr v4.4.0 + Spotflow Device SDK
├── boards/                  — Board-specific Kconfig fragments and DT overlays
│   ├── esp32c3_devkitc.conf          — Minimal coredump mode, reduced heap allocation
│   ├── esp32c3_devkitc.overlay       — Custom 128 KB coredump flash partition
│   ├── esp32c6_devkitc_esp32c6_hpcore.conf
│   ├── esp32c6_devkitc_esp32c6_hpcore.overlay
│   ├── esp32s3_devkitc_esp32s3_procpu.conf
│   └── esp32s3_devkitc_esp32s3_procpu.overlay
└── src/
    ├── main.c               — Sensor polling loop, metric registration, crash injection
    ├── sensor.c             — Simulated I2C sensor driver with injected use-after-free bug
    ├── sensor.h             — Public sensor API
    └── net/                 — Network abstraction (Wi-Fi / Ethernet)
        ├── CMakeLists.txt
        ├── Kconfig
        ├── net.c / net.h    — Entry point: calls Wi-Fi or Ethernet init based on Kconfig
        ├── wifi.c / wifi.h  — Wi-Fi connection and event handling
        └── eth.c / eth.h    — Ethernet DHCP bringup
```

## Related links

- [ESP32 Crash Debugging That Actually Works: Logs, Metrics & AI Analysis with Zephyr](https://feature-blog-post-esp32-zeph.platform-website-9gk.pages.dev/blog/esp32-crash-debugging-zephyr/) — the blog post this example accompanies
- [Fundamentals: Logging](https://docs.spotflow.io/fundamentals/logging?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_sensor_readme&utm_content=link_fundamentals_logging) — log buffering, transport format, runtime filtering
- [Fundamentals: Metrics](https://docs.spotflow.io/fundamentals/metrics?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_sensor_readme&utm_content=link_fundamentals_metrics) — aggregation intervals, system vs. custom metrics
- [Fundamentals: Crash Reports & Core Dumps](https://docs.spotflow.io/fundamentals/crash-reports?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_sensor_readme&utm_content=link_fundamentals_crash) — coredump backend, flash partition setup, AI analysis
- [Fundamentals: Alerts](https://docs.spotflow.io/fundamentals/alerts?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_sensor_readme&utm_content=link_fundamentals_alerts) — alert rules, notification channels
- [Guide: Set Up Alerts](https://docs.spotflow.io/guides/set-up-alerts?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_sensor_readme&utm_content=link_guide_alerts) — step-by-step alert configuration walkthrough
- [Guide: Metrics with Zephyr](https://docs.spotflow.io/guides/zephyr/metrics-zephyr?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_sensor_readme&utm_content=link_guide_metrics_zephyr) — full Zephyr metrics integration reference
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk) — west module source and standalone samples
