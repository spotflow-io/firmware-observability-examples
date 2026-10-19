#!/bin/sh
# Smoke test for the Zephyr virtual device image. CI runs it for every build
# (.github/workflows/build-virtual-device-image.yml), and you can run it locally:
#
#   ci/smoke-test.sh [--boot-only] <image> <platform>
#   ci/smoke-test.sh ghcr.io/spotflow-io/zephyr-virtual-device linux/arm64
#
# The device runs without network access, so the test never contacts the
# Spotflow broker. The device cannot connect, but everything up to the core
# dump upload still runs: the run-time credentials, the demo crash, and the
# restart. --boot-only stops after the credentials, for emulated platforms
# where QEMU itself runs inside a second, slow emulation layer.
set -eu

BOOT_ONLY=0
if [ "${1:-}" = "--boot-only" ]; then
	BOOT_ONLY=1
	shift
fi

if [ "$#" -ne 2 ]; then
	echo "Usage: $0 [--boot-only] <image> <platform>" >&2
	exit 2
fi

IMAGE="$1"
PLATFORM="$2"
NAME="virtual-device-smoke-test-$$"
CREDENTIALS="Run-time credentials loaded (device ID: ci-smoke-test, ingest key: provided)"
CRASH="***** USAGE FAULT *****"
RESTART="Device restarted after a reset"

if [ "$BOOT_ONLY" -eq 1 ]; then
	CRASH_AFTER=0
	WAIT_FOR="$CREDENTIALS"
	TIMEOUT=180
	set -- "$CREDENTIALS"
else
	# The restart follows the crash after about 15 seconds
	CRASH_AFTER=5
	WAIT_FOR="$RESTART"
	TIMEOUT=90
	set -- "$CREDENTIALS" "$CRASH" "$RESTART"
fi

LOG="$(mktemp)"
trap 'docker rm --force "$NAME" >/dev/null 2>&1 || true; rm -f "$LOG"' EXIT

docker run --detach --name "$NAME" --platform "$PLATFORM" --network none \
	--env SPOTFLOW_INGEST_KEY=smoke-test-key \
	--env SPOTFLOW_DEVICE_ID=ci-smoke-test \
	--env SPOTFLOW_CRASH_AFTER="$CRASH_AFTER" \
	"$IMAGE" >/dev/null

elapsed=0
while [ "$elapsed" -lt "$TIMEOUT" ]; do
	docker logs "$NAME" >"$LOG" 2>&1
	if grep -qF "$WAIT_FOR" "$LOG"; then
		break
	fi
	sleep 1
	elapsed=$((elapsed + 1))
done
cat "$LOG"

for marker in "$@"; do
	if ! grep -qF "$marker" "$LOG"; then
		echo "::error::The device log does not contain: $marker"
		exit 1
	fi
done
echo "Smoke test passed on $PLATFORM."
