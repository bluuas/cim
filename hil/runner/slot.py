"""Access to one HIL slot over SWD: flash, reset, read variables from RAM.

Uses OpenOCD with the slot's config from hil/openocd. Variables are found by
name in the ELF's symbol table, so test firmware only has to keep its results
in a global variable (e.g. `state` in examples/config_counter).
"""
import socket
import struct
import subprocess
import tempfile
import time
from pathlib import Path

from elftools.elf.elffile import ELFFile

# where the status display (hil/status-display) picks up its third line
STATUS_FILE = Path("/run/cim-hil/status")


class OpenOcdError(RuntimeError):
    pass


def symbol(elf, name):
    """Address and size of a global variable in an ELF file."""
    with open(elf, "rb") as f:
        symtab = ELFFile(f).get_section_by_name(".symtab")
        if symtab is None:
            raise KeyError(f"{elf} has no symbol table")
        syms = symtab.get_symbol_by_name(name)
        if not syms:
            raise KeyError(f"symbol '{name}' not found in {elf}")
        return syms[0]["st_value"], syms[0]["st_size"]


class Slot:
    def __init__(self, name, openocd_cfg):
        self.name = name
        self.cfg = Path(openocd_cfg)

    def __repr__(self):
        return f"Slot({self.name})"

    def openocd(self, *commands, timeout=60):
        # no GDB/telnet/Tcl servers, so several slots can run in parallel
        args = ["openocd", "-f", str(self.cfg),
                "-c", "gdb_port disabled; telnet_port disabled; tcl_port disabled"]
        args += [a for c in commands for a in ("-c", c)]
        result = subprocess.run(args, capture_output=True, text=True, timeout=timeout)
        if result.returncode != 0:
            raise OpenOcdError(f"slot {self.name}: openocd failed:\n{result.stderr[-2000:]}")
        return result.stderr

    def flash(self, elf):
        out = self.openocd(f"program {elf} verify reset exit", timeout=120)
        if "Verified OK" not in out:
            raise OpenOcdError(f"slot {self.name}: verify failed:\n{out[-2000:]}")

    def reset(self):
        self.openocd("init", "reset run", "shutdown")

    def read(self, address, size):
        """Read memory while the target keeps running."""
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "dump.bin"
            self.openocd("init", f"dump_image {path} 0x{address:08X} {size}", "shutdown")
            return path.read_bytes()

    def read_var(self, elf, name, fmt):
        """Read a global variable and unpack it with struct format `fmt`."""
        address, size = symbol(elf, name)
        if struct.calcsize(fmt) > size:
            raise ValueError(f"format '{fmt}' is larger than '{name}' ({size} bytes)")
        return struct.unpack_from(fmt, self.read(address, struct.calcsize(fmt)))

    def wait_var(self, elf, name, fmt, predicate, timeout=5.0):
        """Poll a variable until predicate(values) is true; return the values."""
        deadline = time.monotonic() + timeout
        while True:
            values = self.read_var(elf, name, fmt)
            if predicate(values) or time.monotonic() > deadline:
                return values
            time.sleep(0.2)

    def rtt_read(self, elf, seconds, symbol_name="cim_log_rtt"):
        """Read the RTT log (firmware/log) for `seconds` while the target runs.

        Starts OpenOCD with its RTT server on a port per slot (9090 + slot index),
        so several slots can be read at the same time. Returns the received text.
        """
        address, _ = symbol(elf, symbol_name)
        port = 9090 + ord(self.name) - ord("a")
        args = ["openocd", "-f", str(self.cfg),
                "-c", "gdb_port disabled; telnet_port disabled; tcl_port disabled",
                "-c", "init",
                "-c", f'rtt setup 0x{address:08X} 16 "SEGGER RTT"',
                "-c", "rtt start",
                "-c", f"rtt server start {port} 0"]
        ocd = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        try:
            deadline = time.monotonic() + 10
            while True:
                try:
                    conn = socket.create_connection(("localhost", port), timeout=1)
                    break
                except OSError:
                    if ocd.poll() is not None or time.monotonic() > deadline:
                        raise OpenOcdError(f"slot {self.name}: RTT server did not start:\n{ocd.stderr.read()[-2000:]}")
                    time.sleep(0.1)
            data = b""
            end = time.monotonic() + seconds
            with conn:
                while (left := end - time.monotonic()) > 0:
                    conn.settimeout(left)
                    try:
                        chunk = conn.recv(4096)
                    except socket.timeout:
                        break
                    if not chunk:
                        break
                    data += chunk
            return data.decode(errors="replace")
        finally:
            ocd.terminate()
            ocd.wait(timeout=5)


def set_status(text):
    """Show text on the bench display, if the display service is installed."""
    try:
        STATUS_FILE.write_text(text + "\n")
    except OSError:
        pass
