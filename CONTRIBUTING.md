# Contributing

Thank you for your interest in contributing to the Spotflow Firmware Observability Examples repository. This guide explains how to add a new example.

## What belongs here

This repository hosts **real-world, story-driven firmware examples** that accompany Spotflow blog posts or tutorials. Each example should:

- Demonstrate a concrete use case that goes beyond the basic SDK samples in [spotflow-io/device-sdk](https://github.com/spotflow-io/device-sdk).
- Be complete and buildable — a reader should be able to clone the repo, follow the example README, and have a working device sending data to Spotflow.
- Be linked to a published (or soon-to-be-published) Spotflow blog post or documentation guide.

## Adding a new example

### 1. Name the example folder

Use kebab-case. The name should describe the **use case**, optionally followed by the **platform** and/or **board** if the example is hardware-specific:

```
<topic>[-<platform>][-<board>]
```

Examples:

| Folder name | When to use |
|---|---|
| `smart-lock-fleet` | Use-case-only, platform-agnostic setup |
| `zephyr-logging-nxp-frdm-rw612` | Platform and board specific |
| `crash-reports-zephyr` | Platform specific, board-agnostic |

### 2. Required files

Every example folder must contain at minimum:

```
<example-name>/
├── README.md           ← required (see template below)
├── CMakeLists.txt      ← required for Zephyr/ESP-IDF examples
├── prj.conf            ← required for Zephyr/nRF Connect examples
├── west.yml            ← required (workspace manifest, or reference one)
└── src/
    └── main.c          ← required
```

For non-Zephyr examples (e.g. bare MQTT), adapt the structure to what is appropriate for the platform.

### 3. Write the example README

Each example must have a standalone `README.md` that lets a developer get from zero to a running device without leaving the repository. Use the following structure:

```markdown
# <Example Title>

> Companion code for the Spotflow blog post:
> **[<Post Title>](<URL>)**

## What this example demonstrates
- Bullet list of features/concepts shown

## Hardware
- List of supported boards
- Any required peripherals (Wi-Fi, Ethernet, etc.)

## Prerequisites
- Zephyr SDK + west (link to install guide)
- Spotflow account and ingest key (link to sign-up)

## Setup
Step-by-step instructions: workspace creation, configuration, build, flash.

## Project structure
Short description of each source file.

## Related links
- Links to relevant Spotflow docs pages
- Link to the Spotflow Device SDK
```

### 4. Add the example to the root README

Add a row to the **Examples** table in [`README.md`](./README.md):

```markdown
| [example-name](./example-name/) | Platform | [Blog Post Title](https://spotflow.io/blog/...) |
```

## Opening a pull request

1. Fork the repository and create a branch from `main`.
2. Add your example following the structure above.
3. Make sure the example builds cleanly before submitting.
4. Open a pull request with a short description of what the example demonstrates and a link to the associated blog post or guide.

## Questions

Reach out on [Discord](https://discord.gg/yw8rAvGZBx) or email [hello@spotflow.io](mailto:hello@spotflow.io).
