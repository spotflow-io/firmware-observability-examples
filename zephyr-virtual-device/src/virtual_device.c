/*
 * Everything that exists only because this device is emulated:
 *
 *  - Run-time credentials. A prebuilt firmware image cannot contain your
 *    ingest key, so QEMU places a small text block in memory before the CPU
 *    starts (-device loader,file=credentials.bin,addr=0x21f00000) and the
 *    firmware hands its values to the Spotflow SDK through the
 *    spotflow_override_*() hooks. When the block is absent, the SDK falls back
 *    to CONFIG_SPOTFLOW_DEVICE_ID and CONFIG_SPOTFLOW_INGEST_KEY as usual.
 *
 *  - A console key instead of a user button.
 *
 *  - A boot counter kept in RAM that survives QEMU's warm reset, so the
 *    automatic demo crash fires once and not in a loop.
 */

#include "virtual_device.h"

#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(virtual_device, LOG_LEVEL_INF);

#define CREDENTIALS_NODE DT_NODELABEL(vdev_credentials)
#define CREDENTIALS_ADDR ((const char *)DT_REG_ADDR(CREDENTIALS_NODE))
#define CREDENTIALS_SIZE DT_REG_SIZE(CREDENTIALS_NODE)
#define CREDENTIALS_MAGIC "SFVDEV1\n"

#define BOOT_MARKER 0x5f0b0075U

static char g_device_id[65];
static char g_ingest_key[257];
static int g_crash_after_seconds = CONFIG_VIRTUAL_DEVICE_CRASH_AFTER_SECONDS;

/* Not zeroed by Zephyr at start-up and not cleared by QEMU on a warm reset. */
static __noinit uint32_t g_boot_marker;
static __noinit uint32_t g_boot_count;
static bool g_first_boot;

static const struct device *const g_console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

/* Copies the value of "key=value\n" from the block into out. */
static void read_value(const char *block, size_t block_len, const char *key, char *out,
		       size_t out_size)
{
	size_t key_len = strlen(key);
	const char *line = block;
	const char *end = block + block_len;

	while (line < end) {
		const char *eol = memchr(line, '\n', end - line);

		if (eol == NULL) {
			eol = end;
		}

		if ((size_t)(eol - line) > key_len && memcmp(line, key, key_len) == 0 &&
		    line[key_len] == '=') {
			size_t value_len = eol - line - key_len - 1;

			if (value_len >= out_size) {
				LOG_ERR("Value of '%s' is too long (%u characters)", key,
					(unsigned int)value_len);
				return;
			}
			memcpy(out, line + key_len + 1, value_len);
			out[value_len] = '\0';
			return;
		}
		line = eol + 1;
	}
}

void virtual_device_init(void)
{
	if (g_boot_marker != BOOT_MARKER) {
		g_boot_marker = BOOT_MARKER;
		g_boot_count = 0;
	}
	g_first_boot = (g_boot_count == 0);
	g_boot_count++;

	const char *block = CREDENTIALS_ADDR;

	if (memcmp(block, CREDENTIALS_MAGIC, strlen(CREDENTIALS_MAGIC)) != 0) {
		LOG_INF("No run-time credentials block, using the values from Kconfig");
		return;
	}

	size_t block_len = strnlen(block, CREDENTIALS_SIZE);
	char crash_after[12] = "";

	read_value(block, block_len, "device_id", g_device_id, sizeof(g_device_id));
	read_value(block, block_len, "ingest_key", g_ingest_key, sizeof(g_ingest_key));
	read_value(block, block_len, "crash_after", crash_after, sizeof(crash_after));

	if (crash_after[0] != '\0') {
		g_crash_after_seconds = atoi(crash_after);
	}

	LOG_INF("Run-time credentials loaded (device ID: %s, ingest key: %s)",
		g_device_id[0] != '\0' ? g_device_id : "<from Kconfig>",
		g_ingest_key[0] != '\0' ? "provided" : "<from Kconfig>");

#ifndef VIRTUAL_DEVICE_SDK_HAS_INGEST_KEY_HOOK
	if (g_ingest_key[0] != '\0') {
		LOG_ERR("This firmware was built with a Spotflow SDK that cannot take the ingest "
			"key at run time. Set CONFIG_SPOTFLOW_INGEST_KEY in credentials.conf and "
			"rebuild.");
	}
#endif
}

bool virtual_device_is_first_boot(void)
{
	return g_first_boot;
}

int virtual_device_crash_after_seconds(void)
{
	return g_crash_after_seconds;
}

bool virtual_device_crash_requested(void)
{
	unsigned char c;
	bool requested = false;

	if (!device_is_ready(g_console)) {
		return false;
	}

	while (uart_poll_in(g_console, &c) == 0) {
		if (c == 'c' || c == 'C') {
			requested = true;
		}
	}

	return requested;
}

/* Spotflow SDK hooks. Returning NULL keeps the value from Kconfig. */

const char *spotflow_override_device_id(void)
{
	return g_device_id[0] != '\0' ? g_device_id : NULL;
}

const char *spotflow_override_ingest_key(void)
{
	return g_ingest_key[0] != '\0' ? g_ingest_key : NULL;
}
