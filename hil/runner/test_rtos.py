"""FreeRTOS integration (#8): examples/rtos_demo runs three tasks; their
counters show the scheduler, the tick and the blocking cim_delay_ms()."""
import time

# examples/rtos_demo/main.c: fast_count, spin_count, led_count, tick_hz, free_heap
STATE = "<5I"
WINDOW = 2.0


def test_rtos_tasks(slot, firmware):
    elf = firmware("rtos_demo")
    slot.flash(elf)
    slot.wait_var(elf, "state", STATE, lambda v: v[0] > 0)

    t0 = time.monotonic()
    fast0, spin0, led0, tick_hz, _ = slot.read_var(elf, "state", STATE)
    time.sleep(WINDOW)
    fast1, spin1, led1, _, free_heap = slot.read_var(elf, "state", STATE)
    dt = time.monotonic() - t0  # includes the SWD reads, so the rates are a bit high

    assert tick_hz == 1000
    # cim_delay_ms(10) per loop: ~100 per second
    assert 80 <= (fast1 - fast0) / dt <= 110, (fast0, fast1, dt)
    # LED task: every 500 ms
    assert 1.5 <= (led1 - led0) / dt <= 2.6, (led0, led1, dt)
    # the low-priority task only runs while "fast" blocks in cim_delay_ms()
    assert spin1 - spin0 > 10000, "spin task starved: cim_delay_ms() busy-waits?"
    assert free_heap > 0
