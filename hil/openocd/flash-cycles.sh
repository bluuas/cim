#!/usr/bin/env bash
# Flash an ELF N times into one HIL slot and count failures (SWD reliability).
# No GDB/telnet/Tcl servers, so several slots can run in parallel.
#   hil/openocd/flash-cycles.sh <slot a|b|c> <file.elf> [count, default 100]
set -euo pipefail

slot=$1 elf=$2 count=${3:-100}
cfg="$(cd "$(dirname "$0")" && pwd)/slot-$slot.cfg"
[[ -f $cfg ]] || { echo "unknown slot '$slot'" >&2; exit 2; }

fail=0
start=$SECONDS
for ((i = 1; i <= count; i++)); do
    if ! out=$(openocd -f "$cfg" \
        -c "gdb_port disabled; telnet_port disabled; tcl_port disabled" \
        -c "program $elf verify reset exit" 2>&1) \
        || ! grep -q "Verified OK" <<<"$out"; then
        fail=$((fail + 1))
        echo "cycle $i: FAILED"
        tail -3 <<<"$out"
    fi
done

elapsed=$((SECONDS - start))
echo "slot $slot: $((count - fail))/$count ok, $fail failed, $((elapsed / count)) s per cycle"
[[ $fail -eq 0 ]]
