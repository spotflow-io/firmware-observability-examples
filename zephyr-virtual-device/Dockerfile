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

# License texts of the firmware's components: the license file of each west
# project and that of picolibc, the C library of the Zephyr SDK. The Ubuntu
# packages of the run-time image already carry theirs in /usr/share/doc.
RUN west list -f '{name} {path}' | grep -v '^manifest ' | while read -r name path; do \
        mkdir -p "/licenses/$name" && cp -r "$path"/LICENSE* "/licenses/$name/" || exit 1; \
    done \
    && cp -r "$ZEPHYR_SDK_INSTALL_DIR/gnu/arm-zephyr-eabi/share/licenses/picolibc" /licenses/

# ---------------------------------------------------------------------------
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends qemu-system-arm \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /workspace/zephyr-virtual-device/build/zephyr/zephyr.elf /opt/virtual-device/zephyr.elf
COPY --from=build /licenses /opt/virtual-device/licenses
COPY run-virtual-device.sh /usr/local/bin/run-virtual-device.sh

ENTRYPOINT ["run-virtual-device.sh"]
