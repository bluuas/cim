"""HAL and TCAN4x5x core driver on hardware (#5): examples/tcan_probe resets the
TCAN, reads its device ID over SPI and sees the power-on interrupt on nINT."""
import time

DEVICE_ID0_TCAN = 0x4E414354  # "TCAN", LSB first

# examples/tcan_probe/main.c: 8 x uint32_t, then 4 x bool
PROBE = "<8I4?"
FIELDS = ("baudrate", "id0", "id1", "revision", "dev_ir", "dev_ir_after", "mode", "nint_count",
          "nint_low_before", "nint_high_after", "vsup_ok", "ok")


def test_tcan_probe(slot, firmware, record_property):
    elf = firmware("tcan_probe")
    slot.flash(elf)
    slot.wait_var(elf, "probe", PROBE, lambda v: v[1] != 0)
    time.sleep(0.2)  # the result is complete a few ms after the ID read
    probe = dict(zip(FIELDS, slot.read_var(elf, "probe", PROBE)))

    # without 12 V on J2 the TCAN reports undervoltage; CAN tests need it, this one does not
    record_property("vsup_ok", probe["vsup_ok"])
    assert probe["id0"] == DEVICE_ID0_TCAN, f"device ID 0x{probe['id0']:08X}"
    assert probe["nint_count"] > 0, "no power-on interrupt on nINT"
    assert probe["ok"], probe
