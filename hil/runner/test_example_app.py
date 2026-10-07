"""Example application (firmware/apps/example, #60): starts FreeRTOS and logs
a heartbeat every second; read over RTT."""
import re


def test_example_app_heartbeat(slot, firmware):
    elf = firmware("example", subdir="apps")
    slot.flash(elf)
    text = slot.rtt_read(elf, seconds=2.5)

    assert re.search(r"I example: example \d+\.\d+\.\d+ starting", text), text
    beats = [int(n) for n in re.findall(r"I example: heartbeat (\d+)", text)]
    assert len(beats) >= 2, text
    assert beats == list(range(beats[0], beats[0] + len(beats))), beats
