#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include <spotflow/ota.h>

#include "metrics/spotflow_metrics_backend.h"
#include "net.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/*
 * Firmware version — bump to "2.0.0" when building the update image.
 * The calibration offset corrects a systematic measurement error identified
 * after the initial field deployment.
 */
#define APP_VERSION             "1.0.0"
#define TEMP_CALIBRATION_OFFSET 0.0f /* v2.0: change to 1.5f */

static struct spotflow_metric_float *g_temperature_metric;

/* ---- OTA phase helpers ---- */

static const char *ota_phase_name(enum spotflow_ota_phase phase)
{
	switch (phase) {
	case SPOTFLOW_OTA_PHASE_NOT_RUNNING:
		return "NOT_RUNNING";
	case SPOTFLOW_OTA_PHASE_PENDING_DOWNLOAD:
		return "PENDING_DOWNLOAD";
	case SPOTFLOW_OTA_PHASE_DOWNLOADING:
		return "DOWNLOADING";
	case SPOTFLOW_OTA_PHASE_PENDING_UPGRADE:
		return "PENDING_UPGRADE";
	case SPOTFLOW_OTA_PHASE_PENDING_REBOOT:
		return "PENDING_REBOOT";
	case SPOTFLOW_OTA_PHASE_UNCONFIRMED:
		return "UNCONFIRMED";
	default:
		return "UNKNOWN";
	}
}

/*
 * confirm_unconfirmed_main_firmware
 *
 * Called once at boot.  If the device started in MCUboot test mode
 * (phase == UNCONFIRMED), run any self-tests here and then confirm the image.
 * If the image is not confirmed before the next reboot, MCUboot rolls back to
 * the previous firmware automatically.
 */
static void confirm_unconfirmed_main_firmware(void)
{
	struct spotflow_ota_main_firmware_state state;
	int ret = spotflow_get_main_firmware_update_state(&state);

	if (ret < 0) {
		LOG_ERR("Failed to query OTA state: %d", ret);
		return;
	}

	if (state.phase != SPOTFLOW_OTA_PHASE_UNCONFIRMED) {
		LOG_INF("OTA phase: %s", ota_phase_name(state.phase));
		return;
	}

	LOG_INF("New firmware booted (phase=%s) — confirming image",
		ota_phase_name(state.phase));

	/*
	 * Insert application-specific self-tests here.
	 * If any test fails, call sys_reboot() instead of confirming.
	 * MCUboot will revert to the previous firmware on the next boot.
	 */

	ret = spotflow_confirm_main_firmware_image(&state);
	if (ret < 0) {
		LOG_ERR("Failed to confirm firmware image: %d", ret);
		return;
	}

	LOG_INF("Firmware v%s confirmed — result will be reported to Spotflow", APP_VERSION);
}

/* ---- OTA callbacks ---- */

/*
 * The SDK calls this on the OTA worker thread whenever the automatic main
 * firmware update moves to a new phase.  Use it to log progress, update an
 * LED, or pause the update when the device is busy.
 */
void spotflow_on_main_firmware_update_progressed(
	const struct spotflow_ota_main_firmware_state *state)
{
	if (state == NULL) {
		return;
	}

	LOG_INF("OTA progress: phase=%s paused=%d result=%d",
		ota_phase_name(state->phase), state->is_paused, state->result);
}

void spotflow_on_update_canceled(void)
{
	LOG_INF("OTA update canceled by the cloud");
}

/* ---- Sensor simulation ---- */

static float read_temperature(void)
{
	/* Simulate a sensor producing values in the 28–34 °C range. */
	float raw = 28.0f + ((float)(sys_rand32_get() % 600)) / 100.0f;
	return raw + TEMP_CALIBRATION_OFFSET;
}

/* ---- Entry point ---- */

int main(void)
{
	LOG_INF("Temperature sensor node v%s starting", APP_VERSION);

	confirm_unconfirmed_main_firmware();

	int ret = spotflow_register_metric_float(
		"temperature_celsius", SPOTFLOW_AGG_INTERVAL_1MIN, &g_temperature_metric);
	if (ret < 0) {
		LOG_ERR("Failed to register temperature_celsius metric: %d", ret);
	}

	/* Allow the network interface driver to finish initialization. */
	k_sleep(K_SECONDS(1));
	spotflow_sample_net_init();

	LOG_INF("Sensor loop running — ready to receive OTA updates");

	while (true) {
		float temp = read_temperature();
		LOG_INF("Temperature: %.1f C", (double)temp);
		(void)spotflow_report_metric_float(g_temperature_metric, temp);
		k_sleep(K_SECONDS(2));
	}

	return 0;
}
