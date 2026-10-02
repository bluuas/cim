# Build and debug environment for the CIM firmware

FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04

ARG PICO_SDK_VERSION=2.3.1
ARG PICOTOOL_VERSION=2.3.1

# Toolchain, build tools and debugging (openocd/gdb for a CMSIS-DAP debug probe)
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        git python3 cmake ninja-build build-essential pkg-config \
        gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib \
        gdb-multiarch openocd libusb-1.0-0-dev usbutils \
    && rm -rf /var/lib/apt/lists/*

# Pico SDK (pinned)
RUN git clone --depth 1 --branch ${PICO_SDK_VERSION} https://github.com/raspberrypi/pico-sdk.git /opt/pico-sdk \
    && git -C /opt/pico-sdk submodule update --init --depth 1
ENV PICO_SDK_PATH=/opt/pico-sdk

# picotool (installed, so the SDK does not rebuild it for every project)
RUN git clone --depth 1 --branch ${PICOTOOL_VERSION} https://github.com/raspberrypi/picotool.git /tmp/picotool \
    && cmake -S /tmp/picotool -B /tmp/picotool/build -G Ninja \
    && cmake --build /tmp/picotool/build \
    && cmake --install /tmp/picotool/build \
    && rm -rf /tmp/picotool
