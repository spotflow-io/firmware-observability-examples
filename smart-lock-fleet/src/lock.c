/*
 * Smart Lock Fleet - Lock Metrics Module
 *
 * Registers and reports three custom Spotflow metrics:
 *
 *   lock_operation_duration_ms  (float, 1-minute aggregation)
 *     Labels: "operation" (lock | unlock), "method" (nfc | keypad | bluetooth)
 *     Reports the time in milliseconds each lock/unlock operation takes.
 *     The SDK aggregates all reports within a 1-minute window and transmits
 *     sum/count/min/max in a single MQTT message.
 *
 *   door_opened  (int, no aggregation)
 *     No labels. Each call to on_door_opened() sends one event immediately.
 *
 *   auth_failure  (int, no aggregation)
 *     Label: "method" (nfc | keypad | bluetooth)
 *     Each failed authentication attempt is sent immediately as a labeled event.
 */

#include "lock.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "metrics/spotflow_metrics_backend.h"

LOG_MODULE_REGISTER(smart_lock, LOG_LEVEL_INF);

/* Metric handles */
static struct spotflow_metric_float *g_op_duration_metric;
static struct spotflow_metric_int   *g_door_opened_metric;
static struct spotflow_metric_int   *g_auth_failure_metric;

/* Possible label values */
static const char *const OPERATIONS[] = { "lock", "unlock" };
static const char *const METHODS[]    = { "nfc", "keypad", "bluetooth" };

/* Simulated duration ranges per method (milliseconds) */
static const float METHOD_DURATION_BASE[]  = { 80.0f,  40.0f,  120.0f }; /* nfc, keypad, bt */
static const float METHOD_DURATION_RANGE[] = { 200.0f, 80.0f,  350.0f };

int init_lock_metrics(void)
{
	int rc;

	/*
	 * lock_operation_duration_ms: float, 1-minute aggregation.
	 * max_timeseries = 6: 2 operations (lock/unlock) x 3 methods = 6 label combinations.
	 * max_labels = 2: "operation" and "method".
	 */
	rc = spotflow_register_metric_float_with_labels(
		"lock_operation_duration_ms",
		SPOTFLOW_AGG_INTERVAL_1MIN,
		6,  /* max_timeseries */
		2,  /* max_labels */
		&g_op_duration_metric);
	if (rc < 0) {
		LOG_ERR("Failed to register lock_operation_duration_ms: %d", rc);
		return rc;
	}
	LOG_INF("Registered metric: lock_operation_duration_ms (float, labeled, 1MIN)");

	/*
	 * door_opened: int, no aggregation.
	 * Every door open event is transmitted immediately as a discrete data point.
	 */
	rc = spotflow_register_metric_int(
		"door_opened",
		SPOTFLOW_AGG_INTERVAL_NONE,
		&g_door_opened_metric);
	if (rc < 0) {
		LOG_ERR("Failed to register door_opened: %d", rc);
		return rc;
	}
	LOG_INF("Registered metric: door_opened (int, NONE)");

	/*
	 * auth_failure: int, no aggregation, labeled by method.
	 * max_timeseries = 3: one series per method (nfc, keypad, bluetooth).
	 * max_labels = 1: "method".
	 */
	rc = spotflow_register_metric_int_with_labels(
		"auth_failure",
		SPOTFLOW_AGG_INTERVAL_NONE,
		3,  /* max_timeseries */
		1,  /* max_labels */
		&g_auth_failure_metric);
	if (rc < 0) {
		LOG_ERR("Failed to register auth_failure: %d", rc);
		return rc;
	}
	LOG_INF("Registered metric: auth_failure (int, labeled, NONE)");

	return 0;
}

/**
 * @brief Report a successful lock/unlock operation with duration and labels.
 */
static void on_lock_operation_complete(const char *operation,
				       const char *method,
				       float duration_ms)
{
	struct spotflow_label labels[] = {
		{ .key = "operation", .value = operation },
		{ .key = "method",    .value = method    },
	};

	int rc = spotflow_report_metric_float_with_labels(
		g_op_duration_metric, duration_ms, labels, 2);
	if (rc < 0) {
		LOG_ERR("Failed to report lock_operation_duration_ms: %d", rc);
	} else {
		LOG_DBG("Reported %s via %s: %.1f ms", operation, method, (double)duration_ms);
	}
}

/**
 * @brief Report a door opened event.
 */
static void on_door_opened(void)
{
	int rc = spotflow_report_event(g_door_opened_metric);
	if (rc < 0) {
		LOG_ERR("Failed to report door_opened: %d", rc);
	} else {
		LOG_DBG("Reported door_opened event");
	}
}

/**
 * @brief Report an authentication failure for a given method.
 */
static void on_auth_failure(const char *method)
{
	struct spotflow_label labels[] = {
		{ .key = "method", .value = method },
	};

	int rc = spotflow_report_event_with_labels(g_auth_failure_metric, labels, 1);
	if (rc < 0) {
		LOG_ERR("Failed to report auth_failure: %d", rc);
	} else {
		LOG_WRN("Auth failure via %s", method);
	}
}

void simulate_lock_operation(void)
{
	/* Pick a random operation and method */
	uint32_t rnd         = sys_rand32_get();
	const char *operation = OPERATIONS[rnd % 2];
	uint32_t method_idx  = (rnd >> 2) % 3;
	const char *method   = METHODS[method_idx];

	/*
	 * Simulate an authentication failure with ~15% probability.
	 * In a real device this would come from the actual auth subsystem.
	 */
	if ((sys_rand32_get() % 100) < 15) {
		on_auth_failure(method);
		/* The user tries again and succeeds; continue to report the operation below. */
	}

	/* Simulate operation duration: base + random fraction of range */
	float base  = METHOD_DURATION_BASE[method_idx];
	float range = METHOD_DURATION_RANGE[method_idx];
	float duration_ms = base + ((float)(sys_rand32_get() % 1000) / 1000.0f) * range;

	on_lock_operation_complete(operation, method, duration_ms);

	/* A successful unlock operation opens the door */
	if (strcmp(operation, "unlock") == 0) {
		on_door_opened();
	}

	LOG_INF("Simulated %s via %s (%.1f ms)", operation, method, (double)duration_ms);
}
