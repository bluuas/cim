#!/usr/bin/env bash
# Set up a Raspberry Pi (Raspberry Pi OS Lite 64-bit, Debian 13) as the CIM HIL host.
# Run on the Pi as the HIL user; it calls sudo where needed. Safe to run again.
# See docs/hil/README.md.
set -euo pipefail

if [[ $EUID -eq 0 ]]; then
    echo "Run as the HIL user, not as root (the user is added to the gpio/i2c groups)." >&2
    exit 1
fi

packages=(
    git
    python3-venv
    openocd     # Raspberry Pi build: has the linuxgpiod adapter and target/rp2040.cfg
    gpiod       # gpioget/gpioset/gpioinfo
    picotool    # flash/reboot over USB (BOOTSEL); udev rule for plugdev comes with Raspberry Pi OS
    i2c-tools   # i2cdetect, for the status display
)

sudo apt-get update
sudo apt-get install -y "${packages[@]}"

# GPIO (SWD, BOOTSEL), I2C, picotool and the CIMs' USB serial without root
sudo usermod -aG gpio,i2c,plugdev,dialout "$USER"

# I2C for the status display; takes effect after a reboot
sudo raspi-config nonint do_i2c 0

# Check that OpenOCD can drive SWD from the Pi's GPIOs
if ! openocd -c "adapter list" -c shutdown 2>&1 | grep -q linuxgpiod; then
    echo "openocd has no linuxgpiod adapter; build it from source with --enable-linuxgpiod" >&2
    exit 1
fi

echo "Done. Reboot once (group membership and I2C): sudo reboot"
