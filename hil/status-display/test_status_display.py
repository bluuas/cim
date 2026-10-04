from status_display import parse_pinctrl, slot_state

# captured on the bench: slot A empty, B and C powered, OpenOCD active on B
PINCTRL = """\
 6: op -- pn | lo // GPIO6 = output
19: ip    pd | hi // GPIO19 = input
21: ip    pd | lo // GPIO21 = input
22: ip    pd | hi // GPIO22 = input
26: ip    pd | lo // GPIO26 = input
27: ip    pd | hi // GPIO27 = input
"""


def test_parse_pinctrl():
    pins = parse_pinctrl(PINCTRL)
    assert pins[6] == ("op", "lo")
    assert pins[22] == ("ip", "hi")
    assert len(pins) == 6


def test_slot_state():
    pins = parse_pinctrl(PINCTRL)
    assert slot_state(pins, 26, 21) == "off"
    assert slot_state(pins, 6, 19) == "swd"
    assert slot_state(pins, 27, 22) == "on"


def test_slot_state_missing_pins():
    assert slot_state({}, 26, 21) == "off"
