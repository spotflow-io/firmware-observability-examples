# syntax=docker/dockerfile:1
#
# One Dockerfile, three uses:
#   --target toolchain   Zephyr build environment
#   --target build       toolchain + this example built for mps2/an385
#   (default)            small run-time image: QEMU + the prebuilt firmware
#
# The firmware is the same whatever machine runs it, so the toolchain and build
# stages always run on the build host's own platform ($BUILDPLATFORM). Only the
# run-time stage is built per target platform, which lets one build publish
# linux/amd64 and linux/arm64 (Apple Silicon) images with an identical ELF.

# ---------------------------------------------------------------------------
FROM --platform=$BUILDPLATFORM ubuntu:24.04 AS toolchain

ARG ZEPHYR_SDK_VERSION=1.0.1
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates cmake device-tree-compiler file git ninja-build \
        python3 python3-pip python3-venv qemu-system-arm wget xz-utils \
    && rm -rf /var/lib/apt/lists/*

# Zephyr SDK: the minimal bundle plus the ARM toolchain only
RUN cd /opt \
    && base=https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION} \
    && wget -q ${base}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-$(uname -m)_minimal.tar.xz \
    && wget -q ${base}/toolchain_gnu_linux-$(uname -m)_arm-zephyr-eabi.tar.xz \
    && tar xf zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-$(uname -m)_minimal.tar.xz \
    && mkdir -p zephyr-sdk-${ZEPHYR_SDK_VERSION}/gnu \
    && tar xf toolchain_gnu_linux-$(uname -m)_arm-zephyr-eabi.tar.xz -C zephyr-sdk-${ZEPHYR_SDK_VERSION}/gnu \
    && rm -rf *.tar.xz zephyr-sdk-${ZEPHYR_SDK_VERSION}/hosttools

ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-${ZEPHYR_SDK_VERSION}

RUN python3 -m venv /opt/venv && /opt/venv/bin/pip install --no-cache-dir west
ENV PATH=/opt/venv/bin:$PATH

# ---------------------------------------------------------------------------
FROM toolchain AS build

WORKDIR /workspace/zephyr-virtual-device
COPY . .

RUN west update --fetch-opt=--depth=1 --narrow \
    && pip install --no-cache-dir -r external/zephyr/scripts/requirements-base.txt

# No credentials.conf here: the image takes the ingest key at run time
RUN west build --pristine --board mps2/an385

# License texts of everything compiled into the firmware, listed in LICENSES.md:
#   - the license files of each west project,
#   - picolibc and libgcc, which come from the Zephyr SDK toolchain,
#   - the notice of each compiled file under a license other than Apache-2.0,
#     for example the BSD-3-Clause files in Zephyr.
RUN mkdir -p /licenses/file-notices \
    && west list -f '{name} {path}' | while read -r name path; do \
           [ "$name" = manifest ] && continue; \
           mkdir -p "/licenses/$name"; \
           cp "$path"/LICENSE* "$path"/COPYING* "/licenses/$name/" 2>/dev/null; \
           [ -n "$(ls "/licenses/$name")" ] || { echo "No license file in $path" >&2; exit 1; }; \
       done \
    && toolchain="$ZEPHYR_SDK_INSTALL_DIR/gnu/arm-zephyr-eabi/share/licenses" \
    && cp -r "$toolchain/picolibc" /licenses/picolibc \
    && mkdir /licenses/libgcc \
    && cp "$toolchain/gcc/COPYING3" "$toolchain/gcc/COPYING.RUNTIME" /licenses/libgcc/ \
    && grep -o '"file": "[^"]*"' build/compile_commands.json | cut -d '"' -f 4 | sort -u \
       | while read -r src; do \
             id="$(grep -m 1 -o 'SPDX-License-Identifier: [^*]*' "$src" | cut -d ' ' -f 2- | sed 's/ *$//')"; \
             case "$id" in "" | Apache-2.0 | "Apache-2.0 OR GPL-2.0-or-later") continue ;; esac; \
             sed -n '1,/^ \*\//p' "$src" > "/licenses/file-notices/$(echo "${src#*/external/}" | tr / _).txt"; \
         done

# ---------------------------------------------------------------------------
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
# Each Ubuntu package keeps its license in /usr/share/doc/<package>/copyright.
# ubuntu-source-packages.txt names the source package and version of each one,
# which is where its source code comes from.
RUN apt-get update && apt-get install -y --no-install-recommends qemu-system-arm \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /opt/virtual-device/licenses \
    && dpkg-query -W -f '${source:Package} ${source:Version}\n' | sort -u \
        > /opt/virtual-device/licenses/ubuntu-source-packages.txt

COPY --from=build /workspace/zephyr-virtual-device/build/zephyr/zephyr.elf /opt/virtual-device/zephyr.elf
COPY --from=build /licenses /opt/virtual-device/licenses
COPY LICENSES.md /opt/virtual-device/licenses/
COPY run-virtual-device.sh /usr/local/bin/run-virtual-device.sh

ENTRYPOINT ["run-virtual-device.sh"]
