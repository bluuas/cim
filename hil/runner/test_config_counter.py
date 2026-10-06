"""Config store on hardware (#9): the boot counter of examples/config_counter
must increase by exactly one per reset, without errors."""

# examples/config_counter/main.c: struct { uint32_t boot_count; uint32_t address; int32_t last_err; } state
STATE = "<IIi"
RESETS = 3


def test_boot_counter_increments_per_reset(slot, firmware):
    elf = firmware("config_counter")
    slot.flash(elf)
    first, _, err = slot.wait_var(elf, "state", STATE, lambda v: v[0] > 0)
    assert first > 0, "firmware did not start (boot_count still 0)"
    assert err == 0

    for i in range(1, RESETS + 1):
        slot.reset()
        count, _, err = slot.wait_var(elf, "state", STATE, lambda v: v[0] == first + i)
        assert (count, err) == (first + i, 0), f"after reset {i}"
