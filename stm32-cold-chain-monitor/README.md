# STM32 Cold-Chain Fridge Monitor with Spotflow

> Companion code for the openstm32.org article:
> **Remote crash debugging for an STM32 fleet with Zephyr RTOS** *(link added on publication)*

A supermarket or pharmacy chain runs a temperature monitor on every refrigerated case, walk-in cooler, and vaccine fridge, hundreds or thousands of them across sites. Each has a local screen and alarm for the on-site staff, so a temperature excursion is caught on site. What headquarters cannot see is the fleet: when a monitor silently stops reporting or crashes, nobody finds out until the stock is spoiled or a compliance log has a hole in it. And you cannot send a technician to every fridge.

This example closes that gap on an [STM32F746G-DISCO](https://www.st.com/en/evaluation-tools/32f746gdiscovery.html). The board runs a small [Zephyr RTOS](https://www.zephyrproject.org/) application that behaves like a cold-chain monitor: it shows the current cabinet temperature on its 4.3" touch LCD, and at the same time streams logs, metrics, and crash core dumps to [Spotflow](https://docs.spotflow.io/?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=stm32_cold_chain_intro) over Ethernet, so the whole fleet is visible from one web interface.

Tested on: **STMicroelectronics STM32F746G-DISCO** (`stm32f746g_disco`), connected over Ethernet, with Zephyr v4.4.0 and Spotflow Device SDK v0.9.0.

## What this example demonstrates

- **Logs** streamed off-device over MQTT/TLS (`LOG_INF`, `LOG_WRN`, `LOG_ERR`)
- **Custom application metrics**: `temperature_celsius` (aggregated on the device to a 1-minute min/max/mean) and `read_errors`
- **System metrics** collected automatically: heap, CPU utilization, network bytes, thread stacks, connection state, and `boot_reset`
- A 60-second **heartbeat**
- **Crash core dumps**: a fatal fault is captured to internal flash, then uploaded to Spotflow on the next boot, where the built-in AI analysis explains the root cause
- A **local LVGL display** on the LTDC LCD showing cabinet temperature, uptime, IP address, probe-error count, and network status (offline / waiting for DHCP / online, tracking the Ethernet carrier); during an excursion the temperature turns amber then red and a red alarm banner appears
- A **touch "SIMULATE EXCURSION" button** (and the physical USER button) to drive the demo

## The crash scenario

The application models a fridge monitor. The cabinet normally sits at 2-6 °C; the safe upper limit is 8 °C. When the temperature crosses that limit, the firmware calls an alarm callback (`g_alarm_callback`) to raise the alert.

The bug: on this unit the callback was never registered, because the cabinet was deployed as a display-only monitor that was never wired to the alerting backend. The firmware calls the callback unconditionally, without a `NULL` check. This is a realistic pattern: a callback registered only on some device variants, but called from a shared processing path. The result is a fault that happens at the worst possible moment, exactly when a fridge warms up and the alarm should fire.

Crash path:

1. Press the physical USER button, or tap **SIMULATE EXCURSION** on the LCD (a door left open or a compressor fault).
2. The simulated cabinet temperature ramps up over a few seconds, past the 8 °C limit. On the LCD the temperature turns amber while warming and red once over the limit, with an alarm banner, so the excursion is visible before the crash.
3. Once over the limit, `check_temperature()` calls `g_alarm_callback(temp)`, a `NULL` function pointer, and the CPU faults (USAGE FAULT, `PC = 0x00000000`).
4. Zephyr writes the core dump to the `coredump-partition` in internal flash.
5. The device reboots, reconnects, and Spotflow uploads the core dump. The crash report with AI analysis is available in the [Events](https://app.spotflow.io/) view within seconds.

## Hardware

[STM32F746G-DISCO](https://www.st.com/en/evaluation-tools/32f746gdiscovery.html): Arm Cortex-M7 at 216 MHz, 320 KB SRAM, 1 MB internal flash, 8 MB external SDRAM, 4.3" 480x272 LCD with FT5336 capacitive touch, and 10/100 Ethernet (LAN8742A PHY).

Connect two cables:

- The **ST-LINK Micro-USB (CN14)**, on the top edge, to your PC. This powers the board and provides flashing, debug, and the serial console (115200 baud).
- An **Ethernet cable (CN9)** to a network with DHCP and internet access.

## Prerequisites

- A Spotflow account and [ingest key](https://docs.spotflow.io/fundamentals/device-authorization?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=stm32_cold_chain_prereq): [sign up for free](https://app.spotflow.io/signup?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=stm32_cold_chain_prereq)
- Git and Python 3.10+
- If you do not already have the Zephyr dependencies installed, follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies) for your operating system.

## Setup

### Step 1: Create the workspace environment

```sh
cd stm32-cold-chain-monitor
python -m venv .venv
# Linux / macOS:
source .venv/bin/activate
# Windows (PowerShell):
# .venv\Scripts\Activate.ps1

pip install west
```

### Step 2: Fetch the pinned dependencies

`.west/config` is committed and points to `west.yml`, so no `west init` step is needed.

```sh
west update
west packages pip --install
```

The manifest uses:

- Zephyr `v4.4.0` (pinned)
- Spotflow Device SDK `main` branch

### Step 3: Install the Zephyr SDK toolchain

```sh
west sdk install --version 1.0.1 --toolchains arm-zephyr-eabi
```

### Step 4: Configure credentials

Copy the template and fill in your ingest key. `credentials.conf` is git-ignored.

```sh
cp credentials-sample.conf credentials.conf
```

```ini
CONFIG_SPOTFLOW_DEVICE_ID="cold-chain-monitor-001"
CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"
```

No Wi-Fi credentials are needed: this board connects over Ethernet and gets its address by DHCP.

## Build and flash

```sh
west build --pristine --board stm32f746g_disco
west flash
```

`west flash` uses STM32CubeProgrammer by default. If you do not have it installed, OpenOCD works too:

```sh
west flash --runner openocd
```

## What to expect on the UART

```
*** Booting Zephyr OS build v4.4.0 ***
[00:00:00.059,000] <inf> main: main: STM32F746G-DISCO cold-chain fridge monitor starting
[00:00:01.059,000] <inf> fridge_monitor: fridge_monitor_init: Cold-chain monitor ready. Press the user button (or the on-screen button) to simulate a temperature excursion.
[00:00:01.065,000] <inf> ui: ui_thread_entry: Cold-chain monitor display ready on the LTDC LCD
[00:00:01.753,000] <inf> phy_mii: check_autonegotiation_completion: PHY (0) Link speed 100 Mb, full duplex
[00:00:01.758,000] <inf> spotflow_sample_eth: handler: Interface is up -> starting DHCPv4
[00:00:08.803,000] <inf> spotflow_device_id: spotflow_get_device_id: Using Spotflow device ID: cold-chain-monitor-001
[00:00:10.364,000] <inf> spotflow_net: spotflow_mqtt_establish_mqtt: MQTT connected!
[00:00:11.060,000] <inf> fridge_monitor: fridge_monitor_step: Cabinet temperature: 2.6 C
```

At the same time, the LCD shows the live cabinet temperature and the red **SIMULATE EXCURSION** button.

## Upload the ELF so crash dumps decode

Before triggering a crash, upload `build/zephyr/zephyr.elf` to Spotflow on the [Firmware Management](https://app.spotflow.io/) page. The build ID is embedded in both the ELF and the core dump, so Spotflow links them automatically. Without the ELF, stack frames show only raw addresses and the AI analysis has no function or variable names to work with.

See [Upload ELF file with symbols](https://docs.spotflow.io/guides/zephyr/crash-reports-zephyr?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=stm32_cold_chain_elf#recommended-upload-elf-file-with-symbols).

## Reproduce the crash

Press the physical **USER** button or tap **SIMULATE EXCURSION** on the LCD. The UART shows the full fault sequence:

```
<wrn> fridge_monitor: fridge_monitor_simulate_excursion: Temperature excursion simulated (door left open). Cabinet warming past the safe limit.
<inf> fridge_monitor: fridge_monitor_step: Cabinet temperature: 7.8 C
<inf> fridge_monitor: fridge_monitor_step: Cabinet temperature: 9.3 C
<wrn> fridge_monitor: check_temperature: Cabinet temperature above safe limit: 9.3 C (limit: 8.0 C)
<err> os: usage_fault: ***** USAGE FAULT *****
<err> os: usage_fault:   Illegal use of the EPSR
<err> os: esf_dump: Faulting instruction address (r15/pc): 0x00000000
<err> os: z_fatal_error: >>> ZEPHYR FATAL ERROR 35: Unknown error on CPU 0
```

After the automatic reboot, the device reconnects and logs `Coredump successfully sent.` The crash then appears in Spotflow with AI analysis.

## Metrics

Custom application metrics:

- `temperature_celsius`: the cabinet temperature, read every 2 s and aggregated on the device into a 1-minute min / max / mean / count before it is sent (`SPOTFLOW_AGG_INTERVAL_1MIN`). One message a minute instead of thirty, and the min/max is what a cold-chain log needs.
- `read_errors`: an event reported on each simulated temperature-probe I2C read timeout

System metrics (no application code required): `heap_free_bytes`, `cpu_utilization_percent`, `network_tx_bytes`, `network_rx_bytes`, `thread_stack_used_percent`, `connection_transport_connected`, `boot_reset`, and more.

## Project structure

- `src/main.c`: entry point. Button setup, network init, monitor init, main loop.
- `src/fridge_monitor.c`: metric registration, temperature simulation, and the alarm/crash path.
- `src/ui.c`: the LVGL display and the on-screen excursion button, running in their own thread.
- `src/spotflow_logo.c`: the Spotflow logo rendered on the display.
- `src/net/`: shared Wi-Fi / Ethernet connectivity helper from the observability examples collection. This board uses the Ethernet path.
- `boards/stm32f746g_disco.overlay`: the internal-flash coredump partition and the application code partition.
- `boards/stm32f746g_disco.conf`: board-specific Kconfig.
- `west.yml`: pinned dependency manifest.
- `credentials-sample.conf`: template for Spotflow credentials.

## Board-specific notes

- **Core dumps go to internal flash.** The last 256 KiB sector of the 1 MB internal flash is reserved for the core dump, and the application is constrained to the first 768 KiB. Internal flash is used rather than the QSPI NOR because core dumps are written from the fatal-error handler, where the QSPI driver's blocking transfers are not available. The internal flash controller programs synchronously and works in that context. Zephyr's flash-partition backend requires the partition to be labelled exactly `coredump-partition`.
- **Thread-mode dumps.** `CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_THREADS` dumps thread stacks and metadata instead of Zephyr's default linker-defined RAM image. The default dumps the full ~187 KiB image, most of which is `.bss`, `.data`, and heap buffers that are often irrelevant to a fault. Thread mode keeps just the faulting stacks and metadata, which stays far smaller than the 256 KiB partition.
- **Config persistence is disabled** on this board (`CONFIG_SPOTFLOW_SAMPLE_CONFIG_PERSISTENCE_FLASH=n`). The only thing it persists is the log level set remotely from Spotflow. The store is initialized from the log backend before the kernel scheduler is running, and the STM32 QSPI flash cannot service a blocking read that early (`settings_subsys_init()` returns `-EDEADLK`). Without it, a remotely set log level is re-applied after each reconnect instead of surviving the reboot. Logs, metrics, and core dumps do not depend on it.
- **The LCD framebuffer and LVGL buffers live in the external SDRAM.** Zephyr's main SRAM region (`sram0`) is the 256 KiB at 0x20010000, with a separate 64 KiB DTCM (320 KB together; ST's datasheet counts 340 KB by also including the instruction-TCM and backup SRAM). Keeping LVGL out of the main region leaves room for the networking and TLS stack.

## Related links

- [Spotflow documentation](https://docs.spotflow.io/?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=stm32_cold_chain_links)
- [Fundamentals: Crash reports and core dumps](https://docs.spotflow.io/fundamentals/monitoring/crash-reports?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=stm32_cold_chain_links)
- [Guide: Crash reports with Zephyr](https://docs.spotflow.io/guides/zephyr/crash-reports-zephyr?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=stm32_cold_chain_links)
- [Fundamentals: Metrics](https://docs.spotflow.io/fundamentals/monitoring/metrics?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=stm32_cold_chain_links)
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk)
