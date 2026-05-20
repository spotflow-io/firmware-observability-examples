# ESP32 Industrial Sensor Observability

> Companion code for the Spotflow blog post:
> **[Why ESP32 Crashes](https://spotflow.io/blog/esp32-remote-logging-monitoring-debugging?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_blog_post)**

This example demonstrates how to instrument an ESP32 industrial sensor node with [Spotflow](https://docs.spotflow.io/?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_docs_intro) so you can investigate one field failure as one workflow built from four complementary signals:

- ESP32 remote logging
- ESP32 device monitoring with custom metrics
- alerting before the reboot
- crash reports after a reboot

The scenario is intentionally concrete: a sensor node starts showing CRC errors, backlog growth, and retry storms before a malformed telemetry frame triggers an application crash.

## Runtime behavior

This example boots through MCUboot, starts the Zephyr application, connects to Wi-Fi, and then connects to Spotflow over [MQTT/TLS](https://docs.spotflow.io/fundamentals/logging?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_fundamentals_logging_mqtt_tls#transport-protocol). Once connected, it continuously streams [remote logs](https://docs.spotflow.io/fundamentals/logging?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_fundamentals_logging_runtime), [system and custom metrics](https://docs.spotflow.io/fundamentals/metrics?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_fundamentals_metrics_runtime), and [crash artifacts](https://docs.spotflow.io/fundamentals/crash-reports?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_fundamentals_crash_reports_runtime). The firmware simulates an industrial sensor node that periodically uploads sensor batches and occasionally emits warning signs such as CRC mismatches and sensor backlog growth. Pressing the user button triggers a deterministic crash path; after reboot, the device reconnects to Wi-Fi and Spotflow and resumes reporting.

## What this example demonstrates

- Zephyr RTOS + Spotflow SDK on ESP32-C3, ESP32-C6, and ESP32-S3
- remote logs over MQTT/TLS
- system metrics plus application metrics in one firmware image
- concrete Spotflow alert rules that trigger before the reboot
- a reproducible button-triggered parser crash
- board-specific coredump storage layout
- one generic application flow across all supported boards

## Hardware

Any supported ESP32 board with Wi-Fi connectivity. Tested on:

- [Espressif ESP32-C3-DevKitC-02](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c3/esp32-c3-devkitc-02/index.html)
- [Espressif ESP32-C6-DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c6/esp32-c6-devkitc-1/index.html)
- [Espressif ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/index.html)

## Prerequisites

**This setup requires:**

- A Spotflow account and [ingest key](https://docs.spotflow.io/fundamentals/device-authorization?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_fundamentals_device_authorization) - [sign up for free](https://app.spotflow.io/signup?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=cta_signup)
- Git
- Python 3.12+

## Setup

### Step 1: Create the workspace environment

```powershell
python -m venv .venv
# Linux/macOS:
source .venv/bin/activate
# Windows (PowerShell):
# .venv/Scripts/Activate.ps1

pip install west
```

### Step 2: Fetch the pinned dependencies

`.west/config` is already committed and points to `west.yml`, so no `west init` step is needed.

> **Windows note:** This workspace can exceed path-length limits during `west update` on Windows if you clone it into a deep directory. Even with long paths enabled in the OS, some tools still fail. Use a short path such as `C:\ws\firmware-observability-examples\esp32-industrial-sensor-observability` when working on Windows.

```sh
west update --fetch-opt=--depth=1 --narrow
west packages pip --install
west blobs fetch hal_espressif --auto-accept
```

The manifest is pinned to:

- Zephyr `v4.4.0`
- Spotflow Device SDK `main`

### Step 3: Install the Zephyr SDK toolchain(s)

ESP32-C3 and ESP32-C6 use the RISC-V toolchain:

```sh
west sdk install --version 1.0.1 --toolchains riscv64-zephyr-elf
```

ESP32-S3 uses the Xtensa toolchain:

```sh
west sdk install --version 1.0.1 --toolchains xtensa-espressif_esp32s3_zephyr-elf
```

### Step 4: Configure the application

Copy `credentials-sample.conf` to `credentials.conf` and fill in your values. This file is excluded from version control by `.gitignore`.

```powershell
Copy-Item credentials-sample.conf credentials.conf
```

Then edit `credentials.conf`:

```ini
CONFIG_NET_WIFI_SSID="<your-ssid>"
CONFIG_NET_WIFI_PASSWORD="<your-password>"
CONFIG_SPOTFLOW_DEVICE_ID="esp32-industrial-sensor-001"
CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"
```

## Build and flash

**Board targets:**

| Board | Target | Notes |
|---|---|---|
| Espressif ESP32-C3 DevKitC | `esp32c3_devkitc` | 4 MB flash, smaller coredump partition |
| Espressif ESP32-C6 DevKitC | `esp32c6_devkitc/esp32c6/hpcore` | 8 MB flash layout |
| Espressif ESP32-S3 DevKitC | `esp32s3_devkitc/esp32s3/procpu` | 32 MB flash override in this example |

```sh
west build --pristine --sysbuild --board <your-board-target>
west flash
```

If your ESP32 flashing setup requires an explicit serial device, pass the appropriate runner option for your environment.

Once the device is running and shows `MQTT connected!` on UART, it is streaming logs, metrics, and crash artifacts to Spotflow. Open [app.spotflow.io/devices](https://app.spotflow.io/devices?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_devices) to see your device appear in the [Device Dashboard](https://docs.spotflow.io/fundamentals/dashboards?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_fundamentals_dashboards_device_dashboard#device-dashboard).

## What to expect on UART

On a healthy boot you should see lines like these:

- `ESP32 industrial sensor node example starting`
- `Connecting to SSID: <your-ssid>`
- `Connected to <your-ssid>`
- `Using Spotflow device ID: <your-device-id>`
- `MQTT connected!`
- `Uploaded sensor batch: channels=3 avg=... age=...`

During steady-state operation the node also emits warning-level signals that model the field failure:

- `Sensor FIFO backlog grew to ... ms after retry storm`
- `CRC mismatch on Modbus frame from remote probe head`

## Crash reproduction

Press the user button on the board to arm the deterministic crash path.

You should then see:

- `User button pressed. Arming reproducible crash path.`
- `Crash repro armed: forcing malformed sensor frame after a short delay`

The application then forces `channel_count = 7` in a frame that only contains three real samples. After the repro path has been armed for a short delay, the parser trusts the corrupted count and dereferences a null pointer in `decode_auxiliary_channel()`.

After reboot, the app reconnects to Wi-Fi and Spotflow and is ready to continue uploading logs, metrics, and crash artifacts.

## Custom metrics

This example registers these application metrics:

- `sensor_cycle_duration_ms`
- `sensor_read_failures`
- `uplink_retry_count`
- `sensor_data_age_ms`
- `application_restarts`

It also enables Spotflow [system metrics](https://docs.spotflow.io/fundamentals/metrics?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_fundamentals_metrics_system_metrics#system-metrics) for:

- heap
- network traffic
- connection state
- reset cause
- thread stack usage

Depending on board support, the generated build can also expose CPU utilization.

## Alerting

This example is designed so that Spotflow alerting can trigger before the crash, while the device is still degrading rather than after it has already rebooted.

Good starter alert rules for this firmware are:

- `sensor_data_age_ms > 1000` for the last 10 minutes, grouped by device ID
- `sensor_read_failures > 0` for the last 10 minutes, grouped by device ID
- `application_restarts > 0` for the last 1 hour, grouped by device ID

These rules match the failure story in the firmware:

- `sensor_data_age_ms` rises when backlog accumulates after retry storms
- `sensor_read_failures` increments when malformed or corrupted frames appear
- `application_restarts` increments after the deterministic crash and reboot

### Set up alert rules in Spotflow

1. Open [Alert Rules](https://app.spotflow.io/alerting/alert-rules?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_alert_rules) in the Spotflow web app.
2. Click `+ Create Alert Rule`.
3. Choose a threshold alert.
4. Build a query for one of the metrics above and group by device ID.
5. Set the evaluation window and threshold:
   - `sensor_data_age_ms > 1000` over the last 10 minutes
   - `sensor_read_failures > 0` over the last 10 minutes
   - `application_restarts > 0` over the last 1 hour
6. Add a notification target with the email addresses that should receive alert notifications.
7. Save the rule and watch the evaluations on the alert-rule detail page.

Start with these thresholds, then tune them for your real device behavior so that the rules catch genuine degradation without creating alert fatigue.

## Project structure

```text
esp32-industrial-sensor-observability/
├── .west/config
├── CMakeLists.txt
├── Kconfig
├── prj.conf
├── west.yml
├── credentials-sample.conf
├── boards/
│   ├── esp32_8M.dtsi
│   ├── esp32c3_devkitc.conf
│   ├── esp32c3_devkitc.overlay
│   ├── esp32c6_devkitc_esp32c6_hpcore.conf
│   ├── esp32c6_devkitc_esp32c6_hpcore.overlay
│   ├── esp32s3_devkitc_esp32s3_procpu.conf
│   └── esp32s3_devkitc_esp32s3_procpu.overlay
└── src/
    ├── main.c
    ├── sensor_node.c
    ├── sensor_node.h
    └── net/
```

## Notes

- The board fragments contain only board-specific technical settings such as flash layout, reconnect behavior, and coredump tuning.
- The ESP32-S3 board in this example overrides the upstream Zephyr `N8` flash geometry to match tested 32 MB hardware.
- This example uses MCUboot via `sysbuild` to avoid the ESP simple-boot ROM `SHA-256 comparison failed ... Attempting to boot anyway...` warning observed during earlier validation.
- If you still see the ROM SHA-256 warning on your hardware, the firmware can still boot and run, but treat it as a boot-flow regression worth investigating.

## Related links

- [Why ESP32 Crashes](https://spotflow.io/blog/esp32-remote-logging-monitoring-debugging?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_blog_post) — the companion blog post
- [Fundamentals: Logging](https://docs.spotflow.io/fundamentals/logging?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_fundamentals_logging) — remote logs and backend behavior
- [Fundamentals: Metrics](https://docs.spotflow.io/fundamentals/metrics?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_fundamentals_metrics) — system vs. custom metrics
- [Fundamentals: Alerts](https://docs.spotflow.io/fundamentals/alerts?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_fundamentals_alerts) — alert conditions, evaluation windows, and notification behavior
- [Guide: Metrics with Zephyr](https://docs.spotflow.io/guides/zephyr/metrics-zephyr?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_guide_metrics_zephyr) — Zephyr integration reference
- [Guide: Set Up Alerts](https://docs.spotflow.io/guides/alert-rules?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_guide_alert_rules) — create alert rules and notification targets in Spotflow
- [Guide: Crash Reports with Zephyr](https://docs.spotflow.io/guides/zephyr/crash-reports-zephyr?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_esp32_industrial_sensor_readme&utm_content=link_guide_crash_reports_zephyr) — crash report integration
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk) — SDK source and samples
