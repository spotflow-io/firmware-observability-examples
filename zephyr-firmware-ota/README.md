# Zephyr Firmware OTA with Spotflow

> Companion code for the Spotflow blog post:
> **[Remote Firmware Updates on Zephyr RTOS with Spotflow OTA](https://spotflow.io/blog/zephyr-ota-frdm-rw612)**

This example shows how to integrate Spotflow OTA updates into a Zephyr RTOS application running on an [NXP FRDM-RW612](https://docs.zephyrproject.org/latest/boards/nxp/frdm_rw612/doc/index.html). With a few lines of code and a single deployment in the Spotflow portal, the device downloads and applies a firmware update over Wi-Fi, without physical access.

Tested on: **NXP FRDM-RW612**. The OTA integration patterns apply to any Zephyr board with MCUboot support.

## What this example demonstrates

- Automatic main firmware OTA via `CONFIG_SPOTFLOW_OTA_AUTO_HANDLE_MAIN_FIRMWARE=y`
- Image confirmation after reboot: `spotflow_confirm_main_firmware_image()`
- MCUboot test mode and automatic rollback on failure
- Progress observation via `spotflow_on_main_firmware_update_progressed()`
- Application metrics: `temperature_celsius` reported every 2 s
- System metrics collected automatically: `boot_reset`, heap, CPU, connection state
- No board overlay required: the FRDM-RW612 DTS already defines MCUboot OTA slots

## Scenario

The application simulates a temperature monitoring node deployed in the field.
**v1.0.0** shipped without a calibration correction (`TEMP_CALIBRATION_OFFSET = 0.0f`).
**v2.0.0** adds the fix (`TEMP_CALIBRATION_OFFSET = 1.5f`), correcting a systematic
measurement error identified after initial deployment. The update is delivered remotely
through the Spotflow portal. No serial cable, no physical visit.

## Hardware

[NXP FRDM-RW612](https://docs.zephyrproject.org/latest/boards/nxp/frdm_rw612/doc/index.html): ARM Cortex-M33, 260 MHz, 1.2 MB SRAM, Wi-Fi 6, 64 MB QSPI flash.

## Prerequisites

- A Spotflow account and [ingest key](https://docs.spotflow.io/fundamentals/device-authorization): [sign up for free](https://app.spotflow.io/signup)
- Git
- Python 3.12+
- If you don't have Zephyr dependencies already installed, refer to the official [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies) for the instructions on how to install them for your operating system.

## Setup

### Step 1: Create the workspace environment

```sh
python -m venv .venv
# Linux / macOS:
source .venv/bin/activate
# Windows (PowerShell):
# .venv\Scripts\Activate.ps1

pip install west
```

### Step 2: Fetch the pinned dependencies

`.west/config` is already committed and points to `west.yml`, so no `west init` step is needed.

> **Windows note:** Use a short path such as `C:\ws\firmware-observability-examples\zephyr-firmware-ota` to avoid path-length issues during `west update`.

```sh
west update --fetch-opt=--depth=1 --narrow
west packages pip --install
```

The manifest is pinned to:

- Zephyr `v4.4.0`
- Spotflow Device SDK `feature/fota` (changes to `main` on release)

### Step 3: Install the Zephyr SDK toolchain

```sh
west sdk install --version 1.0.1 --toolchains arm-zephyr-eabi
```

### Step 4: Download required binary blobs

```sh
# NXP FRDM-RW612: signed Wi-Fi/BLE firmware blobs
west blobs fetch hal_nxp --auto-accept
```

### Step 5: Configure the application

Copy `credentials-sample.conf` to `credentials.conf` and fill in your values:

```sh
cp credentials-sample.conf credentials.conf
```

Then edit `credentials.conf`:

```ini
CONFIG_NET_WIFI_SSID="<your-ssid>"
CONFIG_NET_WIFI_PASSWORD="<your-password>"
CONFIG_SPOTFLOW_DEVICE_ID="nxp-frdm-rw612-ota-001"
CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"
```

## Build and flash v1

Build with sysbuild so MCUboot and the signed application image are generated together:

```sh
west build --sysbuild --pristine --board frdm_rw612 -d build-v1
west flash --build-dir build-v1
```

MCUboot and the application are flashed in two steps automatically.

## Expected serial output on boot

```
*** Booting MCUboot ee39e2d694bd ***
*** Using Zephyr OS build v4.4.0 ***
I: Starting bootloader
I: Primary image: magic=good, swap_type=0x2, copy_done=0x1, image_ok=0x1
I: Secondary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
I: Jumping to the first image slot
*** Booting Zephyr OS build v4.4.0 ***
[00:00:04] <inf> main: main: Temperature sensor node v1.0.0 starting
[00:00:04] <inf> main: confirm_unconfirmed_main_firmware: OTA state: NOT_RUNNING
[00:00:05] <inf> spotflow_sample_net: spotflow_sample_net_init: Initializing Wi-Fi...
[00:00:06] <inf> main: main: Sensor loop running — ready to receive OTA updates
[00:00:06] <inf> main: main: Temperature: 30.5 C
[00:00:08] <inf> main: main: Temperature: 31.6 C
...
[00:00:10] <inf> spotflow_sample_wifi: wifi_event_handler: Connected to <your-ssid>
[00:00:15] <inf> spotflow_net: spotflow_mqtt_establish_mqtt: MQTT connected!
```

## Prepare and deploy firmware v2

Edit `src/main.c` and change:

```c
#define APP_VERSION             "2.0.0"
#define TEMP_CALIBRATION_OFFSET 1.5f /* calibration fix: systematic offset corrected */
```

Rebuild:

```sh
west build --sysbuild --pristine --board frdm_rw612 -d build-v2
```

The OTA image to upload is:

```
build-v2/zephyr-firmware-ota/zephyr/zephyr.signed.bin
```

Follow the [Spotflow OTA deployment guide](https://docs.spotflow.io/guides/ota) to:

1. Upload the signed image to a new firmware version on the [Firmwares](https://app.spotflow.io/firmwares) page.
2. Create a deployment cohort and add your device.
3. Start a new deployment targeting the cohort, and mark the firmware as **Main** in the deployment wizard.

## What to expect during the update

Once a deployment is started, the device downloads the new image and reboots:

```
[...] <inf> main: spotflow_on_main_firmware_update_progressed: OTA progress: phase=PENDING_DOWNLOAD paused=0 result=0
[...] <inf> main: spotflow_on_main_firmware_update_progressed: OTA progress: phase=DOWNLOADING paused=0 result=0
(PENDING_UPGRADE and PENDING_REBOOT are logged just before sys_reboot() and may not appear if the UART buffer does not flush in time)
*** Booting MCUboot ee39e2d694bd ***
*** Using Zephyr OS build v4.4.0 ***
I: Starting bootloader
I: Image index: 0, Swap type: test
I: Starting swap using offset algorithm.
I: Bootloader chainload address offset: 0x20000
*** Booting Zephyr OS build v4.4.0 ***
[...] <inf> main: main: Temperature sensor node v2.0.0 starting
[...] <inf> main: spotflow_on_main_firmware_update_progressed: OTA progress: phase=UNCONFIRMED paused=0 result=0
[...] <inf> main: confirm_unconfirmed_main_firmware: New firmware booted (phase=UNCONFIRMED) — confirming image
[...] <inf> main: confirm_unconfirmed_main_firmware: Firmware v2.0.0 confirmed — result will be reported to Spotflow
[...] <inf> main: main: Temperature: 29.7 C   ← readings now include the 1.5 °C calibration offset
```

The device then reports success to the Spotflow portal and the deployment shows **Succeeded**.

## Project structure

- `src/main.c`: entry point: OTA callbacks, metric registration, sensor loop
- `boards/frdm_rw612.conf`: board-specific Kconfig fragment
- `src/net/`: Wi-Fi / Ethernet connectivity, shared from the observability examples collection
- `west.yml`: pinned dependency manifest (Zephyr v4.4.0, Spotflow Device SDK)
- `sysbuild.conf`: enables MCUboot via Zephyr sysbuild (`SB_CONFIG_BOOTLOADER_MCUBOOT=y`)
- `credentials-sample.conf`: template for Wi-Fi and Spotflow credentials

## Related links

- [Blog post: Remote Firmware Updates on Zephyr RTOS with Spotflow OTA](https://spotflow.io/blog/zephyr-ota-frdm-rw612)
- [Guide: Over-the-air (OTA) updates with Zephyr](https://docs.spotflow.io/guides/zephyr/ota-zephyr)
- [Guide: Deploy Over-the-Air (OTA) Updates](https://docs.spotflow.io/guides/ota)
- [Fundamentals: Over-the-air (OTA) updates](https://docs.spotflow.io/fundamentals/ota)
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk)
