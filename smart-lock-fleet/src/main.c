#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "net.h"
#include "lock.h"

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
