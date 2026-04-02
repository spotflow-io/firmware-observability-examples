# Smart Lock Fleet - Spotflow Custom Metrics Sample

This sample demonstrates custom application metrics for a smart lock fleet using
the [Spotflow](https://spotflow.io) observability platform and Zephyr RTOS.

It accompanies the blog post
[Custom Metrics & Dashboards for Embedded Devices](https://spotflow.io/blog/custom-metrics-dashboards).

## What this sample does

The sample simulates a fleet of smart locks connected to the cloud over MQTT via
the Spotflow device SDK. It registers and reports four custom metrics:

| Metric | Type | Aggregation | Labels |
|---|---|---|---|
| `lock_operation_duration_ms` | float | 1 minute | `operation` (lock/unlock), `method` (nfc/keypad/bluetooth) |
| `door_opened` | int | none (event) | none |
| `auth_failure` | int | none (event) | `method` (nfc/keypad/bluetooth) |
| `battery_level_percent` | float | none | none |

The main loop simulates lock and unlock operations every 3 seconds. The battery
monitor runs in a separate thread and reports battery level every 5 minutes.

## Prerequisites

- [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) installed
- [west](https://docs.zephyrproject.org/latest/develop/west/install.html) installed
- A supported Wi-Fi board (see below)
- A [Spotflow](https://app.spotflow.io/signup) account with a device ID and ingest key

## Supported boards

| Board | Connectivity |
|---|---|
| `frdm_rw612` | Wi-Fi (NXP RW612) |
| `nrf7002dk_nrf5340_cpuapp` | Wi-Fi (nRF7002) |
| `nrf7002dk_nrf5340_cpuapp_ns` | Wi-Fi (nRF7002, TF-M) |
| `esp32c3_devkitm` | Wi-Fi (ESP32-C3) |
| `esp32c6_devkitc_esp32c6_hpcore` | Wi-Fi (ESP32-C6) |
| `esp32s3_devkitc_esp32s3_procpu` | Wi-Fi (ESP32-S3) |
| `rpi_pico_rp2040_w` | Wi-Fi (CYW43) |
| `cy8cproto_062_4343w` | Wi-Fi (AIROC CYW43) |

## Getting started

### 1. Initialize the west workspace

```bash
west init -m https://github.com/spotflow-io/device-sdk --mr main my-workspace
cd my-workspace
west update
```

Alternatively, if you already have a Zephyr workspace with the Spotflow SDK as a
module, copy the `smart_lock_fleet` directory into your workspace and add it to
your west manifest.

### 2. Set credentials

Copy `credentials-sample.conf` to `credentials.conf` and fill in your values:

```conf
CONFIG_NET_WIFI_SSID="YourNetworkName"
CONFIG_NET_WIFI_PASSWORD="YourNetworkPassword"
CONFIG_SPOTFLOW_DEVICE_ID="your-device-id"
CONFIG_SPOTFLOW_INGEST_KEY="your-ingest-key"
```

`credentials.conf` is listed in `.gitignore` and will never be committed.

### 3. Build and flash

```bash
west build -b frdm_rw612 smart_lock_fleet --sysbuild
west flash
```

Replace `frdm_rw612` with your target board. For nRF7002DK:

```bash
west build -b nrf7002dk_nrf5340_cpuapp smart_lock_fleet --sysbuild
west flash
```

## Viewing metrics in Spotflow

Once the device is running and connected, metrics appear in the
[Spotflow portal](https://app.spotflow.io) under **Metrics**.

To build a custom dashboard, navigate to **Dashboards**, click **+ Create
Dashboard**, and add custom widgets for each metric. See
[Create Custom Dashboard](https://docs.spotflow.io/guides/custom-dashboards)
for step-by-step instructions.

## Project structure

```
smart_lock_fleet/
  net-common/          Wi-Fi and Ethernet init helpers (shared across samples)
  boards/              Per-board Kconfig fragments and devicetree overlays
  src/
    main.c             Application entry point
    lock.c / lock.h    Lock metrics: operation duration, door opened, auth failure
    battery.c / ...    Battery monitor thread: battery_level_percent
  CMakeLists.txt
  Kconfig
  Kconfig.sysbuild
  prj.conf
  credentials-sample.conf
  west.yml
```

## Further reading

- [Metrics with Zephyr](https://docs.spotflow.io/guides/zephyr/metrics-zephyr)
- [Metrics with MQTT](https://docs.spotflow.io/guides/mqtt/metrics-mqtt)
- [Custom Dashboards](https://docs.spotflow.io/guides/custom-dashboards)
- [Spotflow metrics sample on GitHub](https://github.com/spotflow-io/device-sdk/tree/main/zephyr/samples/metrics)
