"""Example application (firmware/apps/example, #60 and #11): FreeRTOS,
logging, configuration (node address, boot counter) on hardware."""
import re
import subprocess
import sys
from pathlib import Path

import pytest

CIM_CONFIG = Path(__file__).resolve().parents[2] / "tools" / "cim-config" / "cim_config_swd.py"

# apps/example/app.c: struct { uint32_t beats, address, boot_count; int32_t config_err; } app_state
STATE = "<IIIi"


def app_state(slot, elf):
    beats, address, boot_count, config_err = slot.wait_var(elf, "app_state", STATE, lambda v: v[0] > 0)
    return {"beats": beats, "address": address, "boot_count": boot_count, "config_err": config_err}


def cim_config(slot, *args):
    out = subprocess.run([sys.executable, str(CIM_CONFIG), "--openocd-config", str(slot.cfg), *args],
                         capture_output=True, text=True, check=True)
    return out.stdout


def test_example_app_heartbeat(slot, firmware):
    elf = firmware("example", subdir="apps")
    slot.flash(elf)
    text = slot.rtt_read(elf, seconds=2.5)

    assert re.search(r"I example: example \d+\.\d+\.\d+ starting", text), text
    beats = [int(n) for n in re.findall(r"I example: heartbeat (\d+)", text)]
    assert len(beats) >= 2, text
    assert beats == list(range(beats[0], beats[0] + len(beats))), beats


def test_example_app_boot_count_under_rtos(slot, firmware):
    """The app writes its boot counter to flash while FreeRTOS runs."""
    elf = firmware("example", subdir="apps")
    slot.flash(elf)
    first = app_state(slot, elf)
    assert first["config_err"] == 0, first

    for i in range(1, 3):
        slot.reset()
        state = slot.wait_var(elf, "app_state", STATE, lambda v: v[2] == first["boot_count"] + i)
        assert (state[2], state[3]) == (first["boot_count"] + i, 0), f"after reset {i}: {state}"


def test_example_app_address_persists(slot, firmware):
    """Node address set over SWD (tools/cim-config) is used by the app after a reset."""
    if not CIM_CONFIG.is_file():
        pytest.skip(f"{CIM_CONFIG} not found")
    elf = firmware("example", subdir="apps")
    slot.flash(elf)
    before = app_state(slot, elf)["address"]
    test_address = 200 if before != 200 else 201

    try:
        cim_config(slot, "set-address", str(test_address))
        slot.reset()
        state = slot.wait_var(elf, "app_state", STATE, lambda v: v[1] == test_address)
        assert state[1] == test_address, state
    finally:
        if before:
            cim_config(slot, "set-address", str(before))
