#!/bin/sh
# Starts the Spotflow virtual device: Zephyr firmware on an emulated
# ARM MPS2+ AN385 (Cortex-M3) board in QEMU.
#
#   SPOTFLOW_INGEST_KEY   required   ingest key of your Spotflow workspace
#   SPOTFLOW_DEVICE_ID    optional   default: virtual-device-<hostname>
#   SPOTFLOW_CRASH_AFTER  optional   seconds until the demo crash, default 60, 0 = never
#
#   run-virtual-device.sh elf > zephyr.elf    writes the symbol file to stdout
set -eu

ELF="${VIRTUAL_DEVICE_ELF:-/opt/virtual-device/zephyr.elf}"
QEMU="${QEMU_SYSTEM_ARM:-qemu-system-arm}"

if [ ! -f "$ELF" ]; then
	echo "Firmware not found at $ELF. Set VIRTUAL_DEVICE_ELF to your build/zephyr/zephyr.elf." >&2
	exit 1
fi

if [ "${1:-}" = "elf" ]; then
	exec cat "$ELF"
fi

if [ -z "${SPOTFLOW_INGEST_KEY:-}" ]; then
	echo "SPOTFLOW_INGEST_KEY is not set." >&2
	echo "Create a free account at https://app.spotflow.io/signup and copy an ingest key from https://app.spotflow.io/ingest-keys" >&2
	exit 1
fi

DEVICE_ID="${SPOTFLOW_DEVICE_ID:-virtual-device-$(hostname)}"
CRASH_AFTER="${SPOTFLOW_CRASH_AFTER:-60}"

case "$CRASH_AFTER" in
'' | *[!0-9]*)
	echo "SPOTFLOW_CRASH_AFTER must be a number of seconds, got '$CRASH_AFTER'." >&2
	exit 1
	;;
esac

if [ "${#DEVICE_ID}" -gt 64 ] || [ "${#SPOTFLOW_INGEST_KEY}" -gt 256 ]; then
	echo "SPOTFLOW_DEVICE_ID is limited to 64 characters and SPOTFLOW_INGEST_KEY to 256." >&2
	exit 1
fi

# The firmware image is prebuilt, so it cannot contain your ingest key. QEMU's
# generic loader places this text block in the board's memory before the CPU
# starts, and the firmware reads it at boot (see src/virtual_device.c).
umask 077
CREDENTIALS="$(mktemp "${TMPDIR:-/tmp}/spotflow-virtual-device.XXXXXX")"
trap 'rm -f "$CREDENTIALS"' EXIT
printf 'SFVDEV1\ndevice_id=%s\ningest_key=%s\ncrash_after=%s\n' \
	"$DEVICE_ID" "$SPOTFLOW_INGEST_KEY" "$CRASH_AFTER" >"$CREDENTIALS"

if [ "$(wc -l <"$CREDENTIALS")" -ne 4 ]; then
	echo "The device ID and the ingest key must not contain line breaks." >&2
	exit 1
fi

echo "Spotflow virtual device"
echo "  Device ID:  $DEVICE_ID"
if [ "$CRASH_AFTER" -gt 0 ]; then
	echo "  Demo crash: automatically after $CRASH_AFTER s, or press 'c'"
else
	echo "  Demo crash: press 'c'"
fi
echo "  Open https://app.spotflow.io to watch logs, metrics and the crash report arrive."
echo "  Stop with Ctrl+C."
echo

# QEMU runs as a child so that the credentials file is removed when it exits.
# A background command gets /dev/null as its input, so the console input is
# passed on through descriptor 3 to keep the 'c' key working.
exec 3<&0
"$QEMU" \
	-machine mps2-an385 -cpu cortex-m3 \
	-display none -monitor none -serial stdio \
	-nic user,model=lan9118 \
	-device loader,file="$CREDENTIALS",addr=0x21f00000,force-raw=on \
	-kernel "$ELF" <&3 &
QEMU_PID=$!
exec 3<&-
trap 'kill "$QEMU_PID" 2>/dev/null' TERM INT HUP

STATUS=0
wait "$QEMU_PID" || STATUS=$?
kill "$QEMU_PID" 2>/dev/null || true
exit "$STATUS"
