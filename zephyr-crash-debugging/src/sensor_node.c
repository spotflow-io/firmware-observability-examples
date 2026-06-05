#include "sensor_node.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "metrics/spotflow_metrics_backend.h"

LOG_MODULE_REGISTER(sensor_node, LOG_LEVEL_INF);

/* ---- application metrics ---- */
static struct spotflow_metric_float *g_temperature_metric;
static struct spotflow_metric_int   *g_read_errors_metric;

/* ---- crash path state ---- */

/*
 * Alert callback type.  In a real product this would be set during
 * initialization based on the device configuration (e.g. loaded from NVS or
 * provisioned from the cloud).  On this device variant the callback was never
 * registered because the node was provisioned as a sensor-only unit not wired
 * into an alerting backend.
 *
 * The firmware calls the callback unconditionally whenever the threshold is
 * exceeded — a classic null-function-pointer bug.
 */
typedef void (*alert_fn_t)(float temp_celsius);

static alert_fn_t g_alert_callback = NULL; /* not configured on this device */
static float      g_temp_threshold  = 40.0f; /* °C — safe under normal conditions */
static bool       g_repro_mode      = false;
static int        g_loop_count      = 0;

/* ---- helpers ---- */

static float rand_range(float min, float max)
{
	return min + ((float)(sys_rand32_get() % 1000) / 1000.0f) * (max - min);
}

/*
 * check_threshold — called every sensor cycle when repro mode is armed.
 *
 * BUG: g_alert_callback is NULL (alert handler not registered on this
 * device).  This models a real pattern: a callback is registered only on
 * certain device variants during initialization, but the processing path
 * calls it unconditionally, without a null check.
 */
static void check_threshold(float temp_celsius)
{
	if (temp_celsius > g_temp_threshold) {
		LOG_WRN("Temperature threshold exceeded: %.1f C (threshold: %.1f C)",
			(double)temp_celsius, (double)g_temp_threshold);
		/* NULL dereference — fatal fault on next line */
		g_alert_callback(temp_celsius);
	}
}

/* ---- public API ---- */

int sensor_node_init(void)
{
	int rc;

	g_loop_count = 0;
	g_repro_mode = false;

	rc = spotflow_register_metric_float(
		"temperature_celsius",
		SPOTFLOW_AGG_INTERVAL_NONE,
		&g_temperature_metric);
	if (rc < 0) {
		LOG_ERR("Failed to register temperature_celsius metric: %d", rc);
		return rc;
	}

	rc = spotflow_register_metric_int(
		"read_errors",
		SPOTFLOW_AGG_INTERVAL_NONE,
		&g_read_errors_metric);
	if (rc < 0) {
		LOG_ERR("Failed to register read_errors metric: %d", rc);
		return rc;
	}

	LOG_INF("Sensor node ready. Press the user button to reproduce the crash.");
	return 0;
}

void sensor_node_step(void)
{
	++g_loop_count;

	/* Simulate temperature in 28–34 °C range */
	float temp = rand_range(28.0f, 34.0f);

	int rc = spotflow_report_metric_float(g_temperature_metric, temp);
	if (rc < 0) {
		LOG_ERR("Failed to report temperature_celsius: %d", rc);
	}

	LOG_INF("Sensor reading: %.1f C", (double)temp);

	/* Simulate occasional I2C read errors (every 9th loop) */
	if ((g_loop_count % 9) == 0) {
		LOG_WRN("Sensor I2C read timeout, retrying");
		rc = spotflow_report_event(g_read_errors_metric);
		if (rc < 0) {
			LOG_ERR("Failed to report read_errors: %d", rc);
		}
	}

	/* Crash path: armed by button press */
	if (g_repro_mode && g_loop_count >= 3) {
		check_threshold(temp);
	}
}

void sensor_node_trigger_repro_crash(void)
{
	LOG_WRN("Crash path armed. Lowering temperature threshold to 20.0 C.");
	g_repro_mode     = true;
	g_loop_count     = 0;
	g_temp_threshold = 20.0f; /* below normal range — will trigger on next cycle */
}
