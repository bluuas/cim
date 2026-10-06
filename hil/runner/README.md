# HIL test runner

pytest tests that run firmware on the CIMs of the HIL bench. They run on the Pi: each test flashes a firmware ELF over SWD (`hil/openocd`), resets the board, and reads the firmware's results from RAM.

```mermaid
sequenceDiagram
    participant T as pytest (Pi)
    participant O as OpenOCD (slot-x.cfg)
    participant C as CIM
    T->>O: program app.elf verify reset
    O->>C: flash over SWD
    C->>C: run, write results into a global struct
    T->>O: dump_image &state
    O->>C: read RAM while running
    O-->>T: bytes, unpacked with the struct layout
```

## Bench description

[`hil/bench.toml`](../bench.toml) says what is in each slot. A test that uses the `slot` fixture runs once per slot; empty slots and slots with a known defect are skipped with the reason from the file.

## Fixtures

| Fixture | |
|---|---|
| `slot` | `Slot` of a usable slot: `flash(elf)`, `reset()`, `read(addr, size)`, `read_var(elf, name, fmt)`, `wait_var(elf, name, fmt, predicate)` |
| `firmware` | `firmware("config_counter")` returns `<firmware-dir>/examples/config_counter/config_counter.elf` |

`read_var` finds the variable in the ELF's symbol table and unpacks it with a [`struct`](https://docs.python.org/3/library/struct.html) format, e.g. `"<IIi"` for `struct { uint32_t; uint32_t; int32_t; }`. Test firmware only needs to keep its results in a global variable.

During a test, its name is shown on the bench display ([hil/status-display](../status-display/README.md)).

## Run

On the Pi, from a checkout of the repository with the firmware built (or copied) to `build/cim_proto_v7-debug`:

```sh
python3 -m venv .venv && .venv/bin/pip install -r hil/runner/requirements.txt
cd hil/runner
../../.venv/bin/python -m pytest -v --junitxml=hil-report.xml
```

| Option | Default | |
|---|---|---|
| `--bench` | `hil/bench.toml` | bench description |
| `--firmware-dir` | `build/cim_proto_v7-debug` | build directory with `examples/<name>/<name>.elf` |

## Tests

| Test | Firmware | Checks |
|---|---|---|
| `test_config_counter.py` | `config_counter` | the boot counter in the config store increases by exactly 1 per reset (#9) |
| `test_tcan_probe.py` | `tcan_probe` | TCAN device ID over SPI, power-on interrupt on nINT (#5); VSUP state as JUnit property `vsup_ok` |
