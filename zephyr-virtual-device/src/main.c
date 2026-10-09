#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "net.h"
#include "sensor_node.h"
#include "virtual_device.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define SENSOR_PERIOD_SECONDS 2

int main(void)
{
	LOG_INF("Zephyr virtual device example starting");
	LOG_INF("This is real Zephyr firmware with the Spotflow SDK, running on an emulated Cortex-M3.");

	virtual_device_init();

	/* Allow the network interface driver to finish initialization. */
	k_sleep(K_SECONDS(1));
	spotflow_sample_net_init();

	int rc = sensor_node_init();
	if (rc < 0) {
		LOG_ERR("Failed to initialize sensor node: %d", rc);
		return rc;
	}

	int crash_after = virtual_device_crash_after_seconds();
	bool auto_crash_pending = virtual_device_is_first_boot() && crash_after > 0;

	if (auto_crash_pending) {
		LOG_INF("The demo crash will fire automatically in %d seconds", crash_after);
	} else if (!virtual_device_is_first_boot()) {
		LOG_INF("Device restarted after a reset. A stored core dump is uploaded now.");
	}

	while (true) {
		bool crash_now = virtual_device_crash_requested();

		if (crash_now) {
			LOG_WRN("Key 'c' received. Arming reproducible crash path.");
		}

		if (auto_crash_pending && k_uptime_get() >= (int64_t)crash_after * MSEC_PER_SEC) {
			LOG_WRN("Auto-crash timer expired. Arming reproducible crash path.");
			auto_crash_pending = false;
			crash_now = true;
		}

		if (crash_now) {
			sensor_node_trigger_repro_crash();
		}

		sensor_node_step();
		k_sleep(K_SECONDS(SENSOR_PERIOD_SECONDS));
	}

	return 0;
}
