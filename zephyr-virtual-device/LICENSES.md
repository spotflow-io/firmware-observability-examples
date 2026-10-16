# Licenses

The code of this example is licensed under the MIT License, see [below](#mit-license). The firmware and the Docker image `ghcr.io/spotflow-io/zephyr-virtual-device` also contain third-party software, and each component keeps its own license.

The image contains this file and all license texts in `/opt/virtual-device/licenses`. The [README](./README.md#licenses) shows how to copy them out. Relative paths below start in that folder.

## Firmware

`zephyr.elf` is built from these components:

| Component | License | License text |
| --- | --- | --- |
| This example: `src/`, `boards/` | MIT | [below](#mit-license) |
| [Spotflow Device SDK](https://github.com/spotflow-io/device-sdk) | Business Source License 1.1. The Additional Use Grant permits use only for interacting with the Spotflow Platform. | `spotflow-device-sdk/LICENSE.MD` |
| [Zephyr RTOS](https://github.com/zephyrproject-rtos/zephyr) | Apache-2.0 | `zephyr/LICENSE` |
| [CMSIS 6](https://github.com/zephyrproject-rtos/CMSIS_6) | Apache-2.0 | `cmsis_6/LICENSE` |
| [zcbor](https://github.com/zephyrproject-rtos/zcbor) | Apache-2.0 | `zcbor/LICENSE` |
| [Mbed TLS](https://github.com/zephyrproject-rtos/mbedtls) and [TF-PSA-Crypto](https://github.com/zephyrproject-rtos/tf-psa-crypto) | Apache-2.0 OR GPL-2.0-or-later, used here under Apache-2.0 | `mbedtls/LICENSE`, `tf-psa-crypto/LICENSE` |
| [mldsa-native](https://github.com/zephyrproject-rtos/mldsa-native) | Apache-2.0 OR ISC OR MIT. West fetches it with TF-PSA-Crypto, but this firmware does not link it. | `mldsa-native/LICENSE` |
| picolibc, the C library of the Zephyr SDK | BSD-3-Clause and other BSD-style licenses | `picolibc/` |
| libgcc, from the Zephyr SDK | GPL-3.0-or-later WITH GCC-exception-3.1 | `libgcc/` |

A few compiled files have a license other than that of their project, for example two BSD-3-Clause files in Zephyr. The image build copies the notice of each such file to `file-notices/`.

## Docker image

Besides the firmware, the image contains:

- `run-virtual-device.sh`: MIT, like the rest of this example.
- QEMU (`qemu-system-arm`) and the Ubuntu 24.04 packages it runs on. QEMU as a whole is licensed under GPL-2.0, and the other packages under their own licenses. The copyright and license of each package are in `/usr/share/doc/<package>/copyright`, and the GPL-2.0 text is in `/usr/share/common-licenses/GPL-2`.

`ubuntu-source-packages.txt` lists the source package and version of every installed package. Their source code is in the Ubuntu archive. Download it with `apt-get source <package>=<version>` on Ubuntu 24.04 with source repositories enabled, or from `https://launchpad.net/ubuntu/+source/<package>`.

## MIT License

The license of this example, the same as [the repository's](../LICENSE):

```text
MIT License

Copyright (c) 2026 Spotflow s.r.o.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
