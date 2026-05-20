#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "net.h"
#include "sensor_node.h"

LOG_MODULE_REGISTER(esp32_sensor_main, LOG_LEVEL_INF);

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, { 0 });
static struct gpio_callback button_cb_data;

static int prepare_button(void);

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	LOG_INF("User button pressed. Arming reproducible crash path.");
	sensor_node_trigger_repro_crash();
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
		LOG_ERR("Error %d: failed to configure %s pin %d", ret, button.port->name,
			button.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d", ret,
			button.port->name, button.pin);
		return ret;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	return 0;
}

int main(void)
{
	LOG_INF("ESP32 industrial sensor node example starting");
	LOG_INF("This demo combines ESP32 remote logging, ESP32 device monitoring, and crash reports.");

	int rc = prepare_button();
	if (rc != 0) {
		LOG_ERR("Failed to prepare button: %d", rc);
		return rc;
	}

	/* Allow the network interface driver to finish initialization. */
	k_sleep(K_SECONDS(1));
	spotflow_sample_net_init();

	rc = sensor_node_init();
	if (rc < 0) {
		LOG_ERR("Failed to initialize industrial sensor node: %d", rc);
		return rc;
	}

	while (true) {
		sensor_node_step();
		k_sleep(K_SECONDS(2));
	}

	return 0;
}
