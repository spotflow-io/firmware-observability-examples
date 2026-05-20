#include "sensor_node.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "metrics/spotflow_metrics_backend.h"

LOG_MODULE_REGISTER(sensor_node, LOG_LEVEL_INF);

#define SENSOR_CHANNELS 3
#define SENSOR_READING_MIN_C (-20.0f)
#define SENSOR_READING_MAX_C (110.0f)

static struct spotflow_metric_float *g_sensor_cycle_duration_metric;
static struct spotflow_metric_int *g_sensor_read_failures_metric;
static struct spotflow_metric_int *g_uplink_retry_metric;
static struct spotflow_metric_float *g_sensor_data_age_metric;
static struct spotflow_metric_int *g_application_restarts_metric;

struct telemetry_frame {
	uint8_t channel_count;
	uint16_t sample_period_ms;
	float readings[SENSOR_CHANNELS];
};

static struct telemetry_frame g_frame = {
	.channel_count = SENSOR_CHANNELS,
	.sample_period_ms = 250,
	.readings = { 24.5f, 24.8f, 25.0f },
};

static int g_loop_count;
static int g_sensor_backlog_ms;
static bool g_repro_mode;

static float rand_range(float min, float max)
{
	return min + ((float)(sys_rand32_get() % 1000) / 1000.0f) * (max - min);
}

static int report_application_restart(void)
{
	int rc = spotflow_register_metric_int(
		"application_restarts",
		SPOTFLOW_AGG_INTERVAL_NONE,
		&g_application_restarts_metric);
	if (rc < 0) {
		return rc;
	}

	return spotflow_report_event(g_application_restarts_metric);
}

static int register_metrics(void)
{
	int rc;

	rc = spotflow_register_metric_float(
		"sensor_cycle_duration_ms",
		SPOTFLOW_AGG_INTERVAL_1MIN,
		&g_sensor_cycle_duration_metric);
	if (rc < 0) {
		return rc;
	}

	rc = spotflow_register_metric_int(
		"sensor_read_failures",
		SPOTFLOW_AGG_INTERVAL_NONE,
		&g_sensor_read_failures_metric);
	if (rc < 0) {
		return rc;
	}

	rc = spotflow_register_metric_int(
		"uplink_retry_count",
		SPOTFLOW_AGG_INTERVAL_NONE,
		&g_uplink_retry_metric);
	if (rc < 0) {
		return rc;
	}

	rc = spotflow_register_metric_float(
		"sensor_data_age_ms",
		SPOTFLOW_AGG_INTERVAL_NONE,
		&g_sensor_data_age_metric);
	if (rc < 0) {
		return rc;
	}

	rc = report_application_restart();
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static void report_sensor_cycle(float duration_ms)
{
	int rc = spotflow_report_metric_float(g_sensor_cycle_duration_metric, duration_ms);
	if (rc < 0) {
		LOG_ERR("Failed to report sensor_cycle_duration_ms: %d", rc);
	}
}

static void report_sensor_data_age(float age_ms)
{
	int rc = spotflow_report_metric_float(g_sensor_data_age_metric, age_ms);
	if (rc < 0) {
		LOG_ERR("Failed to report sensor_data_age_ms: %d", rc);
	}
}

static void report_event_metric(struct spotflow_metric_int *metric, const char *metric_name)
{
	int rc = spotflow_report_event(metric);
	if (rc < 0) {
		LOG_ERR("Failed to report %s: %d", metric_name, rc);
	}
}

static void update_sensor_frame(void)
{
	for (int i = 0; i < SENSOR_CHANNELS; ++i) {
		g_frame.readings[i] = rand_range(SENSOR_READING_MIN_C, SENSOR_READING_MAX_C);
	}
}

static float decode_auxiliary_channel(const struct telemetry_frame *frame)
{
	if (frame->channel_count <= SENSOR_CHANNELS) {
		return 0.0f;
	}

	/*
	 * This intentionally models the kind of parser bug that shows up in the field:
	 * the code trusts a corrupted channel count, decides an auxiliary channel must
	 * exist, and dereferences a pointer that was never validated.
	 */
	const float *auxiliary = NULL;

	LOG_ERR("Malformed telemetry frame detected: channel_count=%u exceeds supported=%u",
		frame->channel_count, SENSOR_CHANNELS);

	return *auxiliary;
}

static void process_sensor_frame(const struct telemetry_frame *frame)
{
	float sum = 0.0f;
	uint8_t samples_to_sum = frame->channel_count;

	if (samples_to_sum > SENSOR_CHANNELS) {
		samples_to_sum = SENSOR_CHANNELS;
	}

	for (int i = 0; i < samples_to_sum; ++i) {
		sum += frame->readings[i];
	}

	sum += decode_auxiliary_channel(frame);

	float average = sum / frame->channel_count;
	LOG_INF("Uploaded sensor batch: channels=%u avg=%.2fC age=%d ms",
		frame->channel_count, (double)average, g_sensor_backlog_ms);
}

static void maybe_inject_failures(void)
{
	if ((g_loop_count % 7) == 0) {
		g_sensor_backlog_ms += 350;
		LOG_WRN("Sensor FIFO backlog grew to %d ms after retry storm", g_sensor_backlog_ms);
		report_event_metric(g_uplink_retry_metric, "uplink_retry_count");
	}

	if ((g_loop_count % 11) == 0) {
		LOG_WRN("CRC mismatch on Modbus frame from remote probe head");
		report_event_metric(g_sensor_read_failures_metric, "sensor_read_failures");
	}

	if (g_sensor_backlog_ms > 1500) {
		LOG_ERR("Sensor data age exceeded safe threshold: %d ms", g_sensor_backlog_ms);
	}
}

int sensor_node_init(void)
{
	g_loop_count = 0;
	g_sensor_backlog_ms = 150;
	g_repro_mode = false;

	int rc = register_metrics();
	if (rc < 0) {
		LOG_ERR("Failed to initialize sensor node metrics: %d", rc);
		return rc;
	}

	LOG_INF("Industrial sensor node ready. Press the user button to reproduce the field crash path.");
	return 0;
}

void sensor_node_step(void)
{
	++g_loop_count;

	update_sensor_frame();

	float cycle_duration_ms = rand_range(32.0f, 160.0f) + (float)g_sensor_backlog_ms * 0.05f;
	report_sensor_cycle(cycle_duration_ms);
	report_sensor_data_age((float)g_sensor_backlog_ms);

	maybe_inject_failures();

	if ((g_loop_count % 5) == 0 && g_sensor_backlog_ms > 100) {
		g_sensor_backlog_ms -= 90;
	}

	if (g_repro_mode && g_loop_count >= 3) {
		LOG_ERR("Reproducing field crash: parser trusts corrupted channel_count=%u",
			g_frame.channel_count);
		process_sensor_frame(&g_frame);
		return;
	}

	process_sensor_frame(&g_frame);
}

void sensor_node_trigger_repro_crash(void)
{
	LOG_WRN("Crash repro armed: forcing malformed sensor frame after a short delay");
	g_repro_mode = true;
	g_loop_count = 0;
	g_sensor_backlog_ms = 2100;
	g_frame.channel_count = 7;
	memset(g_frame.readings, 0, sizeof(g_frame.readings));
	for (int i = 0; i < SENSOR_CHANNELS; ++i) {
		g_frame.readings[i] = rand_range(48.0f, 62.0f);
	}
}
