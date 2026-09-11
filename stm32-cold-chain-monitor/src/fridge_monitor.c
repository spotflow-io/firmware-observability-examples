#include "fridge_monitor.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "metrics/spotflow_metrics_backend.h"

LOG_MODULE_REGISTER(fridge_monitor, LOG_LEVEL_INF);

/* Safe upper limit for a fresh-food / vaccine refrigerator. */
#define SAFE_MAX_CELSIUS 8.0f

/* ---- application metrics ---- */
static struct spotflow_metric_float *g_temperature_metric;
static struct spotflow_metric_int   *g_read_errors_metric;

/* ---- shared state for the local display ---- */
static float    g_last_temp   = 0.0f;
static uint32_t g_read_errors = 0;

/* ---- alarm / crash state ---- */

/*
 * Alarm callback type. In a real product this would be registered during
 * initialization based on the unit's configuration (loaded from NVS or
 * provisioned from the cloud). On this cabinet the callback was never
 * registered, because the unit was deployed as a display-only monitor that was
 * never wired to the alerting backend.
 *
 * The firmware calls the callback unconditionally whenever the cabinet leaves
 * its safe range: a classic null-function-pointer bug that faults at the worst
 * possible moment, exactly when a fridge warms up and the alarm should fire.
 */
typedef void (*alarm_fn_t)(float temp_celsius);

static alarm_fn_t g_alarm_callback = NULL; /* not registered on this unit */
static bool       g_excursion      = false;
static int        g_loop_count     = 0;

/* ---- helpers ---- */

static float rand_range(float min, float max)
{
	return min + ((float)(sys_rand32_get() % 1000) / 1000.0f) * (max - min);
}

/*
 * check_temperature: called every cycle.
 *
 * BUG: g_alarm_callback is NULL (alarm handler not registered on this unit).
 * This models a real pattern: a callback registered only on certain device
 * variants during initialization, but called from a shared processing path
 * without a null check.
 */
static void check_temperature(float temp_celsius)
{
	if (temp_celsius > SAFE_MAX_CELSIUS) {
		LOG_WRN("Cabinet temperature above safe limit: %.1f C (limit: %.1f C)",
			(double)temp_celsius, (double)SAFE_MAX_CELSIUS);
		/*
		 * Give the display one refresh to show the alarm state before the
		 * (buggy) handler runs, so on the LCD the fault reads as an alarm and
		 * reboot rather than an instant, unexplained screen blank.
		 */
		k_sleep(K_MSEC(1500));
		/* NULL dereference: fatal fault on next line */
		g_alarm_callback(temp_celsius);
	}
}

/* ---- public API ---- */

int fridge_monitor_init(void)
{
	int rc;

	g_loop_count = 0;
	g_excursion  = false;

	/*
	 * The probe is read every 2 s, but the value is aggregated on the device
	 * and sent once a minute as min / max / mean / count. That is what a
	 * cold-chain log needs, and it keeps the outbound queue small when the
	 * network is down. PT0S (no aggregation) would send 30 messages a minute.
	 */
	rc = spotflow_register_metric_float(
		"temperature_celsius",
		SPOTFLOW_AGG_INTERVAL_1MIN,
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

	LOG_INF("Cold-chain monitor ready. Press the user button (or the on-screen "
		"button) to simulate a temperature excursion.");
	return 0;
}

void fridge_monitor_step(void)
{
	++g_loop_count;

	/*
	 * Simulate the cabinet temperature. Normally 2-6 C. After an excursion is
	 * triggered (door left open / compressor fault) the cabinet warms up
	 * gradually, about 1.5 C per 2 s cycle, so the climb past the 8 C limit is
	 * visible on the LCD for a few seconds before the alarm faults.
	 */
	float temp;
	if (g_excursion) {
		temp = g_last_temp + rand_range(1.2f, 1.8f);
		if (temp > 13.0f) {
			temp = 13.0f;
		}
	} else {
		temp = rand_range(2.0f, 6.0f);
	}
	g_last_temp = temp;

	int rc = spotflow_report_metric_float(g_temperature_metric, temp);
	if (rc < 0) {
		LOG_ERR("Failed to report temperature_celsius: %d", rc);
	}

	LOG_INF("Cabinet temperature: %.1f C", (double)temp);

	/* Simulate an occasional I2C read timeout from the temperature probe. */
	if ((g_loop_count % 9) == 0) {
		LOG_WRN("Temperature probe I2C read timeout, retrying");
		++g_read_errors;
		rc = spotflow_report_event(g_read_errors_metric);
		if (rc < 0) {
			LOG_ERR("Failed to report read_errors: %d", rc);
		}
	}

	/* The alarm path runs every cycle; only an excursion exceeds the limit. */
	check_temperature(temp);
}

void fridge_monitor_simulate_excursion(void)
{
	LOG_WRN("Temperature excursion simulated (door left open). Cabinet warming "
		"past the safe limit.");
	g_excursion = true;
}

float fridge_monitor_last_temp(void)
{
	return g_last_temp;
}

uint32_t fridge_monitor_read_errors(void)
{
	return g_read_errors;
}

bool fridge_monitor_in_excursion(void)
{
	return g_excursion;
}
