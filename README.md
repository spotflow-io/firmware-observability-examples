# Spotflow Firmware Observability Examples

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
[![Docs](https://img.shields.io/badge/docs-docs.spotflow.io-blue)](https://docs.spotflow.io/?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=badge_docs)
[![Blog](https://img.shields.io/badge/blog-spotflow.io%2Fblog-blue)](https://spotflow.io/blog?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=badge_blog)

Real-world firmware examples for the [Spotflow](https://spotflow.io/?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=inline_hero) observability platform. Each example accompanies a Spotflow blog post and provides complete, buildable code that you can clone, adapt, and use as a starting point for your own projects.

---

## What is Spotflow?

[Spotflow](https://spotflow.io/?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=inline_hero) is an observability platform built specifically for embedded devices. It collects logs, metrics, and crash dumps from your firmware and makes them searchable and actionable from a single web interface, without requiring a field visit or a serial cable.

Spotflow integrates with [Zephyr RTOS](https://www.zephyrproject.org/) and [nRF Connect SDK](https://www.nordicsemi.com/Products/Development-software/nRF-Connect-SDK) as a standard west module, and with [ESP-IDF](https://idf.espressif.com/) as an [ESP component](https://components.espressif.com/components/spotflow/device_sdk). Any other platform can connect via MQTT over TLS.

- [Documentation](https://docs.spotflow.io/?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=link_docs)
- [Device SDK](https://github.com/spotflow-io/device-sdk)
- [Sign up for free](https://app.spotflow.io/signup?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=cta_signup)

---

## Examples

| Example | Platform | Blog Post |
|---------|----------|-----------|
| [smart-lock-fleet](./smart-lock-fleet/) | Zephyr RTOS | [Custom Metrics & Dashboards for Embedded Devices](https://spotflow.io/blog/custom-metrics-dashboards-embedded-devices?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=link_blog_smart_lock) |
| [esp32-industrial-sensor-observability](./esp32-industrial-sensor-observability/) | Zephyr RTOS | [Debugging ESP32 Devices in the Field](https://spotflow.io/blog/esp32-remote-logging-monitoring-debugging?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=link_blog_esp32_industrial_sensor) |
| [zephyr-crash-debugging](./zephyr-crash-debugging/) | Zephyr RTOS | [Debugging a Zephyr RTOS Crash with Spotflow AI Analysis](https://spotflow.io/blog/zephyr-crash-debugging?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=link_blog_zephyr_crash) |

---

## Getting Started

1. **Sign up** for a free Spotflow account at [app.spotflow.io/signup](https://app.spotflow.io/signup?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=cta_signup), no credit card required.
2. **Pick an example** from the table above and open its `README.md`.
3. **Follow the build and flash instructions** inside the example to connect your device to Spotflow.

---

## Resources

- [Spotflow Documentation](https://docs.spotflow.io/?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=link_docs)
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk)
- [Spotflow Blog](https://spotflow.io/blog?utm_source=github&utm_medium=referral&utm_campaign=firmware_examples_readme&utm_content=link_blog)
- [Discord Community](https://discord.gg/32sgbECzw)
- [Product Roadmap](https://roadmap.spotflow.io/roadmap)

---

## License

The code in this repository is licensed under the MIT License, see [LICENSE](./LICENSE) for details.
