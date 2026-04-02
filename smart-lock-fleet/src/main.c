/*
 * Smart Lock Fleet - Spotflow Custom Metrics Sample
 *
 * This sample demonstrates custom metrics reporting for a fleet of smart locks
 * using the Spotflow observability platform.
 *
 * Metrics reported:
 *   lock_operation_duration_ms  - duration of each lock/unlock operation (float, 1MIN agg)
 *                                 Labels: "operation" (lock|unlock), "method" (nfc|keypad|bluetooth)
 *   door_opened                 - event fired each time the door opens (int, no agg)
 *   auth_failure                - authentication failure event (int, no agg)
 *                                 Label: "method" (nfc|keypad|bluetooth)
 *   battery_level_percent       - battery percentage sampled every 5 min (float, no agg)
 *
 * Prerequisites:
 *   Copy credentials-sample.conf to credentials.conf and fill in your device ID,
 *   ingest key, and Wi-Fi credentials before building.
 *
 * Build:
 *   west build -b <board> -- -DEXTRA_CONF_FILE=credentials.conf
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "net.h"
#include "lock.h"
#include "battery.h"

LOG_MODULE_REGISTER(smart_lock_main, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("Smart Lock Fleet - Spotflow Metrics Sample");

	/* Allow the network interface driver to finish initialization */
	k_sleep(K_SECONDS(1));

	/* Connect to Wi-Fi or start DHCP on Ethernet, depending on board config */
	spotflow_sample_net_init();

	/* Register lock-related metrics (operation duration, door opened, auth failure) */
	int rc = init_lock_metrics();
	if (rc < 0) {
		LOG_ERR("Failed to initialize lock metrics: %d", rc);
		return -1;
	}

	/*
	 * Start the battery monitor thread.
	 * The thread registers battery_level_percent and reports every 5 minutes.
	 */
	init_battery_monitor();

	LOG_INF("Starting lock operation simulation...");

	/*
	 * Main loop: simulate lock/unlock operations every 3 seconds.
	 *
	 * In a real device this loop would be replaced by interrupt-driven callbacks
	 * from NFC, keypad, or Bluetooth subsystems. The metric reporting calls
	 * (on_lock_operation_complete, on_door_opened, on_auth_failure) are identical
	 * whether called from a simulation loop or real hardware event handlers.
	 */
	while (true) {
		simulate_lock_operation();
		k_sleep(K_SECONDS(3));
	}

	return 0;
}
