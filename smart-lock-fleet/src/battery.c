/*
 * Smart Lock Fleet - Battery Monitor Module
 *
 * Registers and reports one custom Spotflow metric:
 *
 *   battery_level_percent  (float, no aggregation)
 *     No labels. Sampled every 5 minutes and sent immediately as a raw value.
 *
 * The metric is registered inside the thread entry (following the SDK pattern
 * used for the temperature metric in the official metrics sample). The thread
 * starts automatically via K_THREAD_DEFINE and runs independently.
 *
 * In a real device, read_battery_percent() would perform an ADC read and
 * convert the raw voltage to a percentage using the battery discharge curve.
 * Here we simulate a slowly draining battery starting from a random level.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "metrics/spotflow_metrics_backend.h"

LOG_MODULE_REGISTER(smart_lock_battery, LOG_LEVEL_INF);

#define BATTERY_THREAD_STACK_SIZE 2048
#define BATTERY_THREAD_PRIORITY   5

/* Report battery level every 5 minutes */
#define BATTERY_REPORT_INTERVAL_MS (5 * 60 * 1000)

static struct spotflow_metric_float *g_battery_metric;

/**
 * @brief Simulate reading the battery percentage.
 *
 * Starts at a random level between 60% and 100%, then drains by a small
 * random amount each call to mimic real-world battery behaviour.
 *
 * @return Simulated battery level in percent (0.0 – 100.0).
 */
static float read_battery_percent(void)
{
	static float level = -1.0f;

	if (level < 0.0f) {
		/* Initialize to a random starting level on first call */
		level = 60.0f + ((float)(sys_rand32_get() % 4000) / 100.0f);
	}

	/* Drain between 0.05% and 0.25% per 5-minute sample */
	float drain = 0.05f + ((float)(sys_rand32_get() % 20) / 100.0f);
	level -= drain;

	if (level < 0.0f) {
		level = 0.0f;
	}

	return level;
}

static void battery_thread_entry(void)
{
	LOG_INF("Battery monitor thread started");

	/*
	 * Register the metric inside the thread, matching the SDK pattern
	 * (see temperature_thread_entry in the official metrics sample).
	 */
	int rc = spotflow_register_metric_float(
		"battery_level_percent",
		SPOTFLOW_AGG_INTERVAL_NONE,
		&g_battery_metric);
	if (rc < 0) {
		LOG_ERR("Failed to register battery_level_percent: %d", rc);
		return;
	}
	LOG_INF("Registered metric: battery_level_percent (float, NONE)");

	while (true) {
		float level = read_battery_percent();

		rc = spotflow_report_metric_float(g_battery_metric, level);
		if (rc < 0) {
			LOG_ERR("Failed to report battery_level_percent: %d", rc);
		} else {
			LOG_INF("Battery level: %.1f%%", (double)level);
		}

		k_sleep(K_MSEC(BATTERY_REPORT_INTERVAL_MS));
	}
}

K_THREAD_DEFINE(battery_thread, BATTERY_THREAD_STACK_SIZE,
		battery_thread_entry, NULL, NULL, NULL,
		BATTERY_THREAD_PRIORITY, 0, 0);
