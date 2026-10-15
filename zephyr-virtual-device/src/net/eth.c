#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/net_mgmt.h>

LOG_MODULE_REGISTER(spotflow_sample_eth, LOG_LEVEL_INF);

static void handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);

	if (mgmt_event == NET_EVENT_IF_UP) {
		LOG_INF("Interface is up -> starting DHCPv4");
		net_dhcpv4_start(iface);
	}
}

void turn_on_dhcp_when_device_is_up(void)
{
	static struct net_mgmt_event_callback iface_cb;

	net_mgmt_init_event_callback(&iface_cb, handler, NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&iface_cb);

	/*
	 * The interface may already be up (always the case on an emulated board),
	 * in which case NET_EVENT_IF_UP was raised before the callback was added.
	 */
	struct net_if *iface = net_if_get_default();

	if (iface != NULL && net_if_is_up(iface)) {
		LOG_INF("Interface is already up -> starting DHCPv4");
		net_dhcpv4_start(iface);
	}
}
