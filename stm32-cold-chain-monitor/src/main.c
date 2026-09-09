#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "net.h"
#include "fridge_monitor.h"
#include "ui.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, { 0 });
static struct gpio_callback button_cb_data;

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	LOG_WRN("User button pressed. Simulating a temperature excursion.");
	fridge_monitor_simulate_excursion();
}

static int prepare_button(void)
{
	int ret;

	if (!gpio_is_ready_dt(&button)) {
		LOG_ERR("Error: button device %s is not ready", button.port->name);
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d",
			ret, button.port->name, button.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
			ret, button.port->name, button.pin);
		return ret;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	return 0;
}

int main(void)
{
	LOG_INF("STM32F746G-DISCO cold-chain fridge monitor starting");
	LOG_INF("Local display serves on-site staff; Spotflow gives the operations team remote visibility of the fleet.");

	int rc = prepare_button();
	if (rc != 0) {
		LOG_ERR("Failed to prepare button: %d", rc);
		return rc;
	}

	/* Allow the network interface driver to finish initialization. */
	k_sleep(K_SECONDS(1));
	spotflow_sample_net_init();

	rc = fridge_monitor_init();
	if (rc < 0) {
		LOG_ERR("Failed to initialize fridge monitor: %d", rc);
		return rc;
	}

	/* Bring up the local display on the LCD (runs in its own thread). */
	ui_start();

	while (true) {
		fridge_monitor_step();
		k_sleep(K_SECONDS(2));
	}

	return 0;
}
