#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "metrics/spotflow_metrics_backend.h"

#include "net.h"
#include "sensor.h"

LOG_MODULE_REGISTER(industrial_sensor_main, LOG_LEVEL_INF);

/* ── Metric handles ─────────────────────────────────────────────────────── */
static struct spotflow_metric_float *g_temperature_metric;
static struct spotflow_metric_float *g_humidity_metric;
static struct spotflow_metric_int   *g_read_errors_metric;
static struct spotflow_metric_int   *g_crashes_metric;

/* ── Button / crash trigger ─────────────────────────────────────────────── */

/*
 * The BOOT button (sw0 alias) is used to simulate a use-after-free bug on
 * demand.  Pressing it frees the sensor handle while the polling thread still
 * holds a reference; the next sensor_read() call dereferences NULL, triggers
 * a Zephyr kernel panic, and Spotflow uploads the resulting coredump on the
 * next boot for AI-assisted analysis.
 *
 * In production firmware this class of bug typically appears when:
 *   - A watchdog or error-recovery path calls sensor_deinit() while the
 *     polling thread still holds a reference.
 *   - A task priority inversion causes the init/deinit sequence to race with
 *     the read loop.
 */
static const struct gpio_dt_spec g_button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback       g_button_cb;
static atomic_t                   g_crash_requested;

static void button_pressed_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	atomic_set(&g_crash_requested, 1);
	LOG_WRN("BOOT button pressed — crash will be triggered on next sensor read");
}

/* ── Forward declarations ───────────────────────────────────────────────── */
static int  init_sensor_metrics(void);
static void run_sensor_loop(sensor_handle_t *handle);

/* ── main ───────────────────────────────────────────────────────────────── */

int main(void)
{
	LOG_INF("ESP32 Industrial Sensor Node — Spotflow observability example");
	LOG_INF("Logs, metrics, and crash reports all enabled.");

	/* Allow the network interface driver to finish initialization. */
	k_sleep(K_SECONDS(1));

	/* Connect to Wi-Fi (or start DHCP on Ethernet, depending on board). */
	spotflow_sample_net_init();

	/* Register application metrics with Spotflow. */
	int rc = init_sensor_metrics();

	if (rc < 0) {
		LOG_ERR("Failed to initialize sensor metrics: %d", rc);
		return -1;
	}

	/* Initialize the I2C sensor. */
	sensor_handle_t *sensor = sensor_init();

	if (!sensor) {
		LOG_ERR("Sensor initialization failed — aborting");
		return -1;
	}

	/* Configure the BOOT button (sw0) to trigger a crash on press. */
	if (!gpio_is_ready_dt(&g_button)) {
		LOG_ERR("Button GPIO device not ready");
		return -1;
	}

	rc = gpio_pin_configure_dt(&g_button, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Failed to configure button pin: %d", rc);
		return -1;
	}

	/* Wait for strapping-pin glitch to settle before arming the interrupt.
	 * On ESP32-S3 the BOOT button is GPIO0, a strapping pin held low by
	 * the ROM bootloader during SPI_FAST_FLASH_BOOT.  Enabling the
	 * active-low edge interrupt too early fires a spurious ISR before the
	 * user touches anything.  100 ms is enough for the pin to float back
	 * high once the bootloader releases it. */
	k_sleep(K_MSEC(100));

	gpio_init_callback(&g_button_cb, button_pressed_isr, BIT(g_button.pin));
	gpio_add_callback(g_button.port, &g_button_cb);

	rc = gpio_pin_interrupt_configure_dt(&g_button, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc < 0) {
		LOG_ERR("Failed to configure button interrupt: %d", rc);
		return -1;
	}

	LOG_INF("Press the BOOT button to trigger a use-after-free crash and coredump");
	LOG_INF("Starting sensor polling loop");

	run_sensor_loop(sensor);

	/* Never reached in normal operation — the loop ends with a crash. */
	return 0;
}

/* ── Sensor polling loop ────────────────────────────────────────────────── */

static void run_sensor_loop(sensor_handle_t *handle)
{
	sensor_reading_t reading;
	int read_count = 0;
	int consecutive_errors = 0;

	while (true) {
		int rc = sensor_read(handle, &reading);

		if (rc < 0) {
			consecutive_errors++;
			LOG_ERR("Sensor read failed (rc=%d, consecutive errors: %d)",
				rc, consecutive_errors);

			/* Report the error count metric so Spotflow alerting can
			 * fire if the error rate exceeds a threshold. */
			spotflow_report_metric_int(g_read_errors_metric, consecutive_errors);

			if (consecutive_errors >= 5) {
				LOG_ERR("Too many consecutive sensor errors — "
					"reporting crash metric and halting");
				spotflow_report_metric_int(g_crashes_metric, 1);
				k_sleep(K_FOREVER);
			}

			k_sleep(K_SECONDS(5));
			continue;
		}

		consecutive_errors = 0;
		read_count++;

		LOG_INF("Read #%d — temp: %.2f °C, humidity: %.2f %%",
			read_count, reading.temperature_c, reading.humidity_pct);

		/* Report sensor readings as immediate metrics (no aggregation)
		 * so Spotflow shows each individual value on the timeline. */
		spotflow_report_metric_float(g_temperature_metric, reading.temperature_c);
		spotflow_report_metric_float(g_humidity_metric, reading.humidity_pct);

		/* Periodic health log so the log stream shows the device is alive. */
		if (read_count % 10 == 0) {
			LOG_INF("Health check — %d reads completed, "
				"last temp: %.2f °C",
				read_count, reading.temperature_c);
		}

		/*
		 * If the BOOT button was pressed, simulate a use-after-free bug:
		 * free the sensor handle (as a buggy error-recovery path would)
		 * without clearing the pointer held by this thread.
		 *
		 * The warning is logged here — after the read succeeds and
		 * before the next k_sleep — so the log backend has time to
		 * upload it to Spotflow before the kernel panic that occurs at
		 * the start of the next loop iteration.
		 */
		if (atomic_cas(&g_crash_requested, 1, 0)) {
			LOG_WRN("Simulating sensor handle corruption "
				"(use-after-free in error recovery path)");
			sensor_deinit(handle);
			handle = NULL; /* pointer now dangling / NULL */
		}

		k_sleep(K_SECONDS(5));
	}
}

/* ── Metric registration ────────────────────────────────────────────────── */

static int init_sensor_metrics(void)
{
	int rc;

	/*
	 * Temperature — immediate float metric.
	 * Every individual reading is sent to Spotflow without aggregation,
	 * so the full time-series is visible in the dashboard.
	 */
	rc = spotflow_register_metric_float("sensor_temperature_celsius",
					    SPOTFLOW_AGG_INTERVAL_NONE,
					    &g_temperature_metric);
	if (rc < 0) {
		LOG_ERR("Failed to register temperature metric: %d", rc);
		return rc;
	}

	/*
	 * Humidity — immediate float metric.
	 */
	rc = spotflow_register_metric_float("sensor_humidity_percent",
					    SPOTFLOW_AGG_INTERVAL_NONE,
					    &g_humidity_metric);
	if (rc < 0) {
		LOG_ERR("Failed to register humidity metric: %d", rc);
		return rc;
	}

	/*
	 * Sensor read error counter — aggregated over 1 minute.
	 * An alert in the Spotflow dashboard fires when the sum of
	 * sensor_read_errors over a 5-minute window exceeds a threshold,
	 * giving the on-call engineer an early warning before the device
	 * crashes completely.
	 */
	rc = spotflow_register_metric_int("sensor_read_errors",
					  SPOTFLOW_AGG_INTERVAL_1MIN,
					  &g_read_errors_metric);
	if (rc < 0) {
		LOG_ERR("Failed to register read_errors metric: %d", rc);
		return rc;
	}

	/*
	 * Crash event counter — aggregated over 1 minute.
	 * Any non-zero value in the Spotflow dashboard means the device
	 * detected an unrecoverable error.  Combined with the coredump
	 * uploaded on the next boot, this metric pins the exact timestamp
	 * of the incident.
	 */
	rc = spotflow_register_metric_int("sensor_crashes",
					  SPOTFLOW_AGG_INTERVAL_1MIN,
					  &g_crashes_metric);
	if (rc < 0) {
		LOG_ERR("Failed to register crashes metric: %d", rc);
		return rc;
	}

	LOG_INF("Sensor metrics registered: temperature, humidity, "
		"read_errors, crashes");
	return 0;
}
