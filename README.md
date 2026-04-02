# Spotflow Firmware Observability Examples

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
[![Docs](https://img.shields.io/badge/docs-docs.spotflow.io-blue)](https://docs.spotflow.io/)
[![Discord](https://img.shields.io/discord/1372202003635114125?label=Discord&logo=discord&logoColor=white)](https://discord.gg/32sgbECzw)
[![Blog](https://img.shields.io/badge/blog-spotflow.io%2Fblog-blue)](https://spotflow.io/blog)

Real-world firmware examples for the [Spotflow](https://spotflow.io/) observability platform. Each example accompanies a Spotflow blog post and provides complete, buildable code that you can clone, adapt, and use as a starting point for your own projects.

---

## What is Spotflow?

[Spotflow](https://spotflow.io/) is an observability platform built specifically for embedded devices. It collects logs, metrics, and crash dumps from your firmware and makes them searchable and actionable from a single web interface, without requiring a field visit or a serial cable.

Spotflow integrates with [Zephyr RTOS](https://www.zephyrproject.org/), [nRF Connect SDK](https://www.nordicsemi.com/Products/Development-software/nRF-Connect-SDK), and [ESP-IDF](https://idf.espressif.com/) as a standard west module. Any other platform can connect via MQTT over TLS.

- [Documentation](https://docs.spotflow.io/)
- [Device SDK](https://github.com/spotflow-io/device-sdk)
- [Sign up for free](https://app.spotflow.io/signup)

---

## Examples

| Example | Platform | Blog Post |
|---------|----------|-----------|
| [smart-lock-fleet](./smart-lock-fleet/) | Zephyr RTOS | [Custom Metrics & Dashboards for Embedded Devices](https://spotflow.io/blog/custom-metrics-dashboards-embedded-devices) |

---

## Getting Started

1. **Sign up** for a free Spotflow account at [app.spotflow.io/signup](https://app.spotflow.io/signup), no credit card required.
2. **Pick an example** from the table above and open its `README.md`.
3. **Follow the build and flash instructions** inside the example to connect your device to Spotflow.

---

## Resources

- [Spotflow Documentation](https://docs.spotflow.io/)
- [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk)
- [Spotflow Blog](https://spotflow.io/blog)
- [Discord Community](https://discord.gg/yw8rAvGZBx)
- [Product Roadmap](https://roadmap.spotflow.io/roadmap)

---

## Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md) for instructions on how to add a new example.

## License

The code in this repository is licensed under the MIT License, see [LICENSE](./LICENSE) for details.
