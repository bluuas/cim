"""CAN FD between two CIMs on the terminated HAT bus (#7): one sends frames
with a counter (examples/can_tx_counter), the other echoes them with ID + 1
(examples/can_echo). 10 000 frames of 64 bytes at 500 kbit/s / 2 Mbit/s, no
loss. Needs +BATT on J2 (12 V to 24 V, JP1 open), see docs/hil/README.md."""
import pytest

FRAMES = 10000

# examples/can_tx_counter/main.c
TX_STATE = "<i20I"
TX_FIELDS = ("init_err", "sent", "send_busy", "send_err", "echoes", "echo_bad", "echo_gaps", "other_rx",
             "period_us", "len", "tx_errors", "rx_errors", "bus_off", "rx_lost", "rx_overflows",
             "last_error", "last_data_error", "dev_ir", "empty_sent", "empty_echoes", "empty_bad")
# examples/can_echo/main.c
ECHO_STATE = "<i13I"
ECHO_FIELDS = ("init_err", "received", "echoed", "send_busy", "send_err", "last_id",
               "tx_errors", "rx_errors", "bus_off", "rx_lost", "rx_overflows", "last_error", "last_data_error", "dev_ir")

TCAN_ERR_NO_VSUP = -4


def read(slot, elf, fmt, fields):
    return dict(zip(fields, slot.read_var(elf, "state", fmt)))


def test_can_echo(slot_pair, firmware, record_property):
    tx_slot, echo_slot = slot_pair
    tx_elf, echo_elf = firmware("can_tx_counter"), firmware("can_echo")

    echo_slot.flash(echo_elf)
    echo = read(echo_slot, echo_elf, ECHO_STATE, ECHO_FIELDS)
    if echo["init_err"] == TCAN_ERR_NO_VSUP:
        pytest.skip("no VSUP: CAN needs 12 V to 24 V on J2 with JP1 open")
    assert echo["init_err"] == 0, f"{echo_slot}: tcan_init failed ({echo['init_err']})"

    tx_slot.flash(tx_elf)
    tx = read(tx_slot, tx_elf, TX_STATE, TX_FIELDS)
    assert tx["init_err"] == 0, f"{tx_slot}: tcan_init failed ({tx['init_err']})"

    duration = FRAMES * tx["period_us"] / 1e6
    tx = dict(zip(TX_FIELDS, tx_slot.wait_var(tx_elf, "state", TX_STATE,
                                               lambda v: v[1] >= FRAMES, timeout=duration + 10)))
    echo = read(echo_slot, echo_elf, ECHO_STATE, ECHO_FIELDS)
    record_property("sent", tx["sent"])
    record_property("echoes", tx["echoes"])

    assert tx["sent"] >= FRAMES, tx
    assert tx["bus_off"] == 0 and echo["bus_off"] == 0, (tx, echo)
    assert tx["send_err"] == 0 and tx["send_busy"] == 0, tx
    assert echo["send_err"] == 0, echo
    assert echo["rx_lost"] == 0 and echo["rx_overflows"] == 0, echo
    assert tx["rx_lost"] == 0 and tx["rx_overflows"] == 0, tx
    assert tx["echo_bad"] == 0 and tx["echo_gaps"] == 0, tx
    # echoes still in flight at the time of the read
    assert tx["sent"] - tx["echoes"] <= 2, tx
    # frames without payload (DLC 0, #34) come back with length 0
    assert tx["empty_sent"] > 0 and tx["empty_bad"] == 0, tx
    assert tx["empty_sent"] - tx["empty_echoes"] <= 2, tx
    # one frame may sit between received++ and echoed++ at the time of the read
    assert echo["received"] >= FRAMES and echo["received"] - echo["echoed"] <= 1, echo
    assert tx["tx_errors"] == 0 and tx["rx_errors"] == 0, tx
    assert echo["tx_errors"] == 0 and echo["rx_errors"] == 0, echo
