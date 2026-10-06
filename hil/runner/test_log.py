"""Logging (#10): examples/log_demo logs a counter every 100 ms; the lines are
read over SWD with RTT."""
import re

LINE = re.compile(r"^\[ *(\d+)\.(\d{3})\] ([EWID]) demo: (.*)$")


def test_log_over_rtt(slot, firmware):
    elf = firmware("log_demo")
    slot.flash(elf)
    text = slot.rtt_read(elf, seconds=2.0)
    lines = [l for l in text.splitlines() if l]
    assert len(lines) >= 10, text

    counts = []
    for line in lines:
        m = LINE.match(line)
        assert m, f"unexpected line: {line!r}"
        level, msg = m.group(3), m.group(4)
        assert level != "D", "DEBUG is compiled out at the default level"
        if level == "I":
            counts.append(int(msg.split()[1]))
        else:
            n = int(msg.split()[1])
            assert level == "W" and n % 10 == 0, line

    # the counter has no gaps: RTT drops whole lines only when its buffer is full
    assert counts == list(range(counts[0], counts[0] + len(counts))), counts
