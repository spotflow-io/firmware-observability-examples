#include "sensor.h"

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(sensor, LOG_LEVEL_DBG);

/*
 * Internal sensor state.  In a real driver this would hold the I2C device
 * pointer, calibration coefficients, etc.
 */
struct sensor_handle {
	/** Simulated I2C address of the sensor. */
	uint8_t i2c_addr;
	/** Sequence number — used to inject the crash after N reads. */
	uint32_t read_count;
};

sensor_handle_t *sensor_init(void)
{
	sensor_handle_t *h = k_malloc(sizeof(sensor_handle_t));

	if (!h) {
		LOG_ERR("Failed to allocate sensor handle");
		return NULL;
	}

	h->i2c_addr = 0x44; /* Simulated SHT4x address */
	h->read_count = 0;

	LOG_INF("Sensor initialized (I2C addr 0x%02x)", h->i2c_addr);
	return h;
}

int sensor_read(sensor_handle_t *handle, sensor_reading_t *out)
{
	/*
	 * Dereference `handle` unconditionally — if the caller passes NULL
	 * (which happens after sensor_deinit() in main.c), the CPU faults
	 * and Zephyr captures a coredump.
	 *
	 * A real driver would do `if (!handle) return -EINVAL;`, but we
	 * deliberately omit that guard to demonstrate the crash scenario.
	 */
	handle->read_count++;

	/* Simulate temperature: 18–30 °C with small random variation */
	uint32_t rand_temp = sys_rand32_get() % 1200; /* 0–11.99 */
	out->temperature_c = 18.0 + (double)rand_temp / 100.0;

	/* Simulate relative humidity: 40–70 % */
	uint32_t rand_hum = sys_rand32_get() % 3000; /* 0–29.99 */
	out->humidity_pct = 40.0 + (double)rand_hum / 100.0;

	LOG_DBG("Read %u: temp=%.2f°C hum=%.2f%%",
		handle->read_count, out->temperature_c, out->humidity_pct);

	return 0;
}

void sensor_deinit(sensor_handle_t *handle)
{
	if (!handle) {
		return;
	}

	LOG_INF("Deinitializing sensor (total reads: %u)", handle->read_count);
	k_free(handle);
}
