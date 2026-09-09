#include "ui.h"

#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#include <lvgl.h>

#include "fridge_monitor.h"

LOG_MODULE_REGISTER(ui, LOG_LEVEL_INF);

/* Spotflow logo mark, generated from the official logo.svg (see spotflow_logo.c). */
extern const lv_image_dsc_t spotflow_logo;

#define UI_THREAD_STACK_SIZE 4096
#define UI_THREAD_PRIORITY   6
#define UI_REFRESH_MS        500

static lv_obj_t *temp_label;
static lv_obj_t *uptime_label;
static lv_obj_t *ip_label;
static lv_obj_t *errors_label;
static lv_obj_t *status_label;

/*
 * On-screen "SIMULATE EXCURSION" button. Drives the same temperature excursion
 * as the physical USER button, so the crash-to-Spotflow demo can be run entirely
 * from the touchscreen.
 */
static void crash_btn_cb(lv_event_t *e)
{
	ARG_UNUSED(e);
	LOG_WRN("On-screen button pressed. Simulating a temperature excursion.");
	fridge_monitor_simulate_excursion();
}

static void build_ui(void)
{
	lv_obj_t *scr = lv_screen_active();
	lv_obj_set_style_bg_color(scr, lv_color_hex(0x0b1f33), LV_PART_MAIN);

	/* Centered Spotflow logo. */
	lv_obj_t *logo = lv_image_create(scr);
	lv_image_set_src(logo, &spotflow_logo);
	lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 12);

	/* Tagline below the logo. */
	lv_obj_t *tagline = lv_label_create(scr);
	lv_label_set_text(tagline, "Firmware Observability. Automated.");
	lv_obj_set_style_text_color(tagline, lv_color_hex(0x8fa3b8), 0);
	lv_obj_align(tagline, LV_ALIGN_TOP_MID, 0, 62);

	temp_label = lv_label_create(scr);
	lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(temp_label, lv_color_hex(0x35d07f), 0);
	lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 12, 92);
	lv_label_set_text(temp_label, "--.- C");

	uptime_label = lv_label_create(scr);
	lv_obj_set_style_text_color(uptime_label, lv_color_hex(0xcfd8e3), 0);
	lv_obj_align(uptime_label, LV_ALIGN_TOP_LEFT, 12, 134);

	ip_label = lv_label_create(scr);
	lv_obj_set_style_text_color(ip_label, lv_color_hex(0xcfd8e3), 0);
	lv_obj_align(ip_label, LV_ALIGN_TOP_LEFT, 12, 160);

	errors_label = lv_label_create(scr);
	lv_obj_set_style_text_color(errors_label, lv_color_hex(0xcfd8e3), 0);
	lv_obj_align(errors_label, LV_ALIGN_TOP_LEFT, 12, 186);

	status_label = lv_label_create(scr);
	lv_obj_set_style_text_color(status_label, lv_color_hex(0xffcc00), 0);
	lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 12, 212);
	lv_label_set_text(status_label, "Network: waiting for DHCP...");

	lv_obj_t *device = lv_label_create(scr);
	lv_label_set_text(device, "Device: cold-chain-monitor-001");
	lv_obj_set_style_text_color(device, lv_color_hex(0x5b6f84), 0);
	lv_obj_align(device, LV_ALIGN_BOTTOM_LEFT, 12, -14);

	lv_obj_t *btn = lv_button_create(scr);
	lv_obj_set_size(btn, 200, 64);
	lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -14, -14);
	lv_obj_set_style_bg_color(btn, lv_color_hex(0xd21f1f), 0);
	lv_obj_add_event_cb(btn, crash_btn_cb, LV_EVENT_CLICKED, NULL);

	lv_obj_t *btn_label = lv_label_create(btn);
	lv_label_set_text(btn_label, "SIMULATE EXCURSION");
	lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), 0);
	lv_obj_center(btn_label);
}

static void update_ui(void)
{
	char buf[40];

	snprintf(buf, sizeof(buf), "%.1f C", (double)fridge_monitor_last_temp());
	lv_label_set_text(temp_label, buf);

	uint32_t up = (uint32_t)(k_uptime_get() / 1000);
	snprintf(buf, sizeof(buf), "Uptime: %02u:%02u:%02u",
		 up / 3600U, (up % 3600U) / 60U, up % 60U);
	lv_label_set_text(uptime_label, buf);

	snprintf(buf, sizeof(buf), "Probe errors: %u", fridge_monitor_read_errors());
	lv_label_set_text(errors_label, buf);

	char ipstr[NET_IPV4_ADDR_LEN] = "-";
	bool online = false;
	struct net_if *iface = net_if_get_default();
	if (iface != NULL) {
		struct in_addr *addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
		if (addr != NULL) {
			net_addr_ntop(AF_INET, addr, ipstr, sizeof(ipstr));
			online = true;
		}
	}
	snprintf(buf, sizeof(buf), "IP: %s", ipstr);
	lv_label_set_text(ip_label, buf);

	/*
	 * The SDK has no public "MQTT connected" query, so the display reports what
	 * it can see for itself: whether the interface has an address.
	 */
	if (online) {
		lv_label_set_text(status_label, "Network: online");
		lv_obj_set_style_text_color(status_label, lv_color_hex(0x35d07f), 0);
	}
}

static void ui_thread_entry(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	/* The display initializes at APPLICATION level; give it a moment if needed. */
	for (int i = 0; i < 50 && !device_is_ready(display_dev); i++) {
		k_sleep(K_MSEC(20));
	}
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready; local panel disabled");
		return;
	}

	build_ui();
	display_blanking_off(display_dev);
	LOG_INF("Cold-chain monitor display ready on the LTDC LCD");

	uint32_t last_update = 0;
	while (true) {
		uint32_t now = k_uptime_get_32();
		if ((now - last_update) >= UI_REFRESH_MS) {
			update_ui();
			last_update = now;
		}
		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}
}

K_THREAD_STACK_DEFINE(ui_stack, UI_THREAD_STACK_SIZE);
static struct k_thread ui_thread;

void ui_start(void)
{
	k_thread_create(&ui_thread, ui_stack, K_THREAD_STACK_SIZEOF(ui_stack),
			ui_thread_entry, NULL, NULL, NULL,
			UI_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&ui_thread, "ui");
}
