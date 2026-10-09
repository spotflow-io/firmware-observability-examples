# Spotflow Virtual Device: Zephyr RTOS on QEMU, No Hardware Needed

This example runs real Zephyr RTOS firmware with the [Spotflow](https://docs.spotflow.io/) device SDK on an emulated ARM Cortex-M3 board. You need no development board, no debug probe, and, if you use the Docker image, no toolchain. Logs and metrics reach your Spotflow workspace seconds after the device starts, and a crash report with AI analysis follows about a minute later.

Runs on: **QEMU, machine `mps2-an385`** (ARM MPS2+ AN385). The firmware is the same code you would flash to a physical board. Only the network interface, the flash, and the way credentials arrive are emulated.

## What this example demonstrates

- Log collection over MQTT/TLS from a device that exists only in QEMU
- Application metrics: `temperature_celsius` and `read_errors`
- System metrics collected automatically: heap, CPU, stack usage per thread, connection state
- Zephyr core dump collection via `CONFIG_SPOTFLOW_COREDUMPS=y`, stored in a flash partition and uploaded after the reboot
- **Spotflow AI Crash Analysis** of a deterministic crash that you trigger with a key press or a timer
- A fleet of virtual devices started with one `docker compose` command

## Crash scenario

The scenario is the same as in [zephyr-crash-debugging](../zephyr-crash-debugging). The application simulates a temperature monitoring node. An alert callback (`g_alert_callback`) is never registered on this device variant, and the firmware calls it unconditionally when the temperature exceeds the threshold.

Crash path:

1. You press `c` in the console, or the auto-crash timer expires
2. The threshold is lowered to 20 °C, below the simulated 28–34 °C range
3. After 3 loop iterations, `check_threshold()` calls `g_alert_callback(temp)` → `NULL` function pointer → usage fault
4. Zephyr saves the core dump to the `coredump-partition` in the simulated flash
5. The device resets, reconnects, and uploads the core dump

The automatic crash fires once per QEMU run. After the reset the device keeps running, so you can also watch normal telemetry.

## Option 1: Run the prebuilt image

### Prerequisites

- A Spotflow account and [ingest key](https://docs.spotflow.io/fundamentals/device-authorization): [sign up for free](https://app.spotflow.io/signup)
- Docker

### Build and run

```sh
docker build -t spotflow-virtual-device .
docker run --rm -it -e SPOTFLOW_INGEST_KEY="<your-ingest-key>" spotflow-virtual-device
```

The first command builds the firmware inside the image, which takes a few minutes. The second one starts the device.

| Variable | Default | Meaning |
| --- | --- | --- |
| `SPOTFLOW_INGEST_KEY` | required | Ingest key of your workspace |
| `SPOTFLOW_DEVICE_ID` | `virtual-device-<hostname>` | Device ID shown in Spotflow |
| `SPOTFLOW_CRASH_AFTER` | `60` | Seconds until the demo crash. `0` disables the timer. |

Press `c` to trigger the crash at any time. Stop the device with `Ctrl+C`.

### Upload the ELF file

Copy the symbol file out of the image and upload it to Spotflow before you look at the crash report. Without it, stack frames show only raw addresses.

```sh
docker run --rm spotflow-virtual-device elf > zephyr.elf
```

See [Upload ELF file with symbols](https://docs.spotflow.io/guides/zephyr/crash-reports-zephyr#recommended-upload-elf-file-with-symbols) for the upload steps. Spotflow links the ELF to the core dump using the build ID embedded in both.

### Run a fleet

```sh
SPOTFLOW_INGEST_KEY="<your-ingest-key>" docker compose up --build --scale device=5
```

Each replica gets its own device ID from the container hostname. The Free plan covers 25 devices.

## Option 2: Build the firmware yourself

Use this path when you want to change the firmware. It follows the same steps as the other examples in this repository.

### Prerequisites

- A Spotflow account and [ingest key](https://docs.spotflow.io/fundamentals/device-authorization): [sign up for free](https://app.spotflow.io/signup)
- Git
- Python 3.12+
- QEMU with `qemu-system-arm` (the example was run on versions 8.2 and 10.0)
- If you don't have Zephyr dependencies already installed, refer to the official [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies) for the instructions on how to install them for your operating system.

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

```sh
west update --fetch-opt=--depth=1 --narrow
west packages pip --install
```

The manifest is pinned to:

- Zephyr `v4.4.0`
- Spotflow Device SDK `feature/dynamic-ingest-key` (temporary, until `CONFIG_SPOTFLOW_INGEST_KEY_DYNAMIC` is merged to `main`)

### Step 3: Install the Zephyr SDK toolchain

```sh
west sdk install --version 1.0.1 --toolchains arm-zephyr-eabi
```

### Step 4: Configure the application

Copy `credentials-sample.conf` to `credentials.conf` and fill in your values:

```sh
cp credentials-sample.conf credentials.conf
```

```ini
CONFIG_SPOTFLOW_DEVICE_ID="zephyr-virtual-device-001"
CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"
```

### Step 5: Build and run

```sh
west build --pristine --board mps2/an385
west build -t run
```

`west build -t run` starts QEMU with the credentials from `credentials.conf`. Press `c` to trigger the crash. Exit QEMU with `Ctrl+A`, then `X`.

Upload `build/zephyr/zephyr.elf` to Spotflow as described above.

To pass the ingest key at run time instead, build without `credentials.conf`. The build then picks up `runtime-credentials.conf`, which enables `CONFIG_SPOTFLOW_INGEST_KEY_DYNAMIC`, and the launcher script supplies the key:

```sh
VIRTUAL_DEVICE_ELF=build/zephyr/zephyr.elf SPOTFLOW_INGEST_KEY="<your-ingest-key>" ./run-virtual-device.sh
```

## What to expect on the console

```
Spotflow virtual device
  Device ID:  virtual-device-vm
  Demo crash: automatically after 20 s, or press 'c'
  Open https://app.spotflow.io to watch logs, metrics and the crash report arrive.
  Stop with Ctrl+C.

*** Booting Zephyr OS build 684c9e8f32e4 ***
[00:00:02.360,000] <inf> main: main: Zephyr virtual device example starting
[00:00:02.360,000] <inf> virtual_device: read_credentials_block: Run-time credentials loaded (device ID: virtual-device-vm, ingest key: provided)
[00:00:03.370,000] <inf> spotflow_sample_eth: turn_on_dhcp_when_device_is_up: Interface is already up -> starting DHCPv4
[00:00:03.370,000] <inf> main: main: The demo crash will fire automatically in 20 seconds
[00:00:03.370,000] <inf> sensor_node: sensor_node_step: Sensor reading: 29.5 C
[00:00:04.760,000] <inf> spotflow_net: spotflow_mqtt_establish_mqtt: MQTT connected!
[00:00:05.380,000] <inf> sensor_node: sensor_node_step: Sensor reading: 30.7 C
...
[00:00:21.460,000] <wrn> main: main: Auto-crash timer expired. Arming reproducible crash path.
[00:00:21.460,000] <wrn> sensor_node: sensor_node_trigger_repro_crash: Crash path armed. Lowering temperature threshold to 20.0 C.
[00:00:25.480,000] <wrn> sensor_node: check_threshold: Temperature threshold exceeded: 32.3 C (threshold: 20.0 C)
[00:00:25.480,000] <err> os: usage_fault: ***** USAGE FAULT *****
[00:00:25.490,000] <err> spotflow_coredump: k_sys_fatal_error_handler: Halting system
*** Booting Zephyr OS build 684c9e8f32e4 ***
[00:00:03.370,000] <inf> main: main: Device restarted after a reset. A stored core dump is uploaded now.
[00:00:08.700,000] <inf> spotflow_net: spotflow_mqtt_establish_mqtt: MQTT connected!
[00:00:12.070,000] <inf> spotflow_coredump: spotflow_coredumps_thread_entry: All coredump chunks enqueued
[00:00:12.390,000] <inf> spotflow_coredump: spotflow_poll_and_process_enqueued_coredump_chunks: Coredump successfully sent.
```

The crash report appears in the [Events](https://app.spotflow.io/) page after the last line.

## How the emulation works

- **Network.** QEMU user-mode networking (`CONFIG_NET_QEMU_USER=y`) gives the emulated LAN9118 Ethernet controller outbound TCP through the host. The device gets its address over DHCP and opens a normal MQTT/TLS connection to `mqtt.spotflow.io:8883`. No root access or TAP interface is needed.
- **Flash.** QEMU's `mps2-an385` machine has 16 MiB of PSRAM at `0x21000000` that Zephyr's board definition does not use. `boards/mps2_an385.overlay` moves the board's simulated flash there and adds the settings and core dump partitions. RAM content survives a reset in QEMU, so the core dump written before the reboot is still there afterwards. It does not survive a QEMU restart.
- **Credentials.** A prebuilt firmware image cannot contain your ingest key. The launcher script writes the device ID and the key to a small text block, and QEMU's generic loader places it at `0x21f00000` before the CPU starts. `src/virtual_device.c` parses the block and hands the device ID to the SDK through `spotflow_override_device_id()` and the ingest key through `spotflow_on_ingest_key_requested()`, which the SDK calls when `CONFIG_SPOTFLOW_INGEST_KEY_DYNAMIC=y`. The script deletes the file when QEMU exits.
- **Time.** `CONFIG_QEMU_ICOUNT=n` makes the emulated clock follow wall-clock time. With instruction counting enabled, QEMU fast-forwards idle time and network timeouts fire early.

## Limitations

- **Not a security reference.** The emulated board has no entropy source, so the example enables `CONFIG_TEST_RANDOM_GENERATOR`, which is not cryptographically secure. The TLS server certificate is still verified. Never copy this setting into a product.
- **Reset cause is unknown.** The board has no hardware info driver, so the `boot_reset` metric reports `UNKNOWN`.
- **No OTA updates.** The example does not include MCUboot.
- **The ingest key comes from one place.** The SDK takes it either from Kconfig or from the application, never both. Firmware built with `credentials.conf` ignores a key passed at run time and logs a warning. Firmware built without it does not connect until the launcher provides a key.

## Project structure

- `src/main.c`: entry point: network init, crash triggers, main sensor loop
- `src/sensor_node.c`: metric registration, sensor simulation, and the crash path
- `src/virtual_device.c`: run-time credentials, console key, boot counter
- `src/net/`: Wi-Fi / Ethernet connectivity, shared from the observability examples collection
- `boards/mps2_an385.conf`: QEMU networking and clock settings
- `boards/mps2_an385.overlay`: simulated flash in PSRAM, core dump partition, credentials block
- `run-virtual-device.sh`: launcher that passes credentials to QEMU
- `Dockerfile`: toolchain, firmware build, and the run-time image in three stages
- `compose.yaml`: a fleet of virtual devices
- `west.yml`: pinned dependency manifest (Zephyr v4.4.0, Spotflow Device SDK)
- `credentials-sample.conf`: template for build-time Spotflow credentials
- `runtime-credentials.conf`: enables the run-time ingest key when `credentials.conf` does not exist

## Related links

- [Quickstart: Zephyr Integration](https://docs.spotflow.io/quickstart/zephyr)
- [Guide: Crash reports with Zephyr](https://docs.spotflow.io/guides/zephyr/crash-reports-zephyr)
- [Fundamentals: Crash reports & core dumps](https://docs.spotflow.io/fundamentals/monitoring/crash-reports)
- [Fundamentals: Metrics](https://docs.spotflow.io/fundamentals/monitoring/metrics)
- [Example: zephyr-crash-debugging](../zephyr-crash-debugging), the same scenario on an NXP FRDM-RW612
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk)
