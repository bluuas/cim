"""Fixtures for HIL tests. See README.md."""
import tomllib
from pathlib import Path

import pytest

from slot import Slot, set_status

HIL_DIR = Path(__file__).resolve().parent.parent
REPO_DIR = HIL_DIR.parent


def pytest_addoption(parser):
    parser.addoption("--bench", type=Path, default=HIL_DIR / "bench.toml",
                     help="bench description (default: hil/bench.toml)")
    parser.addoption("--firmware-dir", type=Path, default=REPO_DIR / "build" / "cim_proto_v7-debug",
                     help="firmware build directory (default: build/cim_proto_v7-debug)")


def load_bench(config):
    path = config.getoption("--bench")
    with open(path, "rb") as f:
        slots = tomllib.load(f)["slots"]
    for s in slots.values():
        s["openocd"] = path.parent / s["openocd"]
    return slots


def pytest_generate_tests(metafunc):
    # every test that uses `slot` runs once per slot of the bench
    if "slot" in metafunc.fixturenames:
        names = sorted(load_bench(metafunc.config))
        metafunc.parametrize("slot", names, indirect=True)


@pytest.fixture
def slot(request):
    """A usable slot; absent or defective ones are skipped with the reason."""
    name = request.param
    info = load_bench(request.config)[name]
    if not info.get("present", False):
        pytest.skip(f"slot {name} empty: {info.get('note', 'not present')}")
    if "defect" in info:
        pytest.skip(f"slot {name}: {info['defect']}")
    return Slot(name, info["openocd"])


@pytest.fixture
def firmware(request):
    """firmware('config_counter') -> examples/config_counter/config_counter.elf;
    firmware('example', subdir='apps') -> apps/example/example.elf."""
    build = request.config.getoption("--firmware-dir")

    def find(name, subdir="examples"):
        elf = build / subdir / name / f"{name}.elf"
        if not elf.is_file():
            pytest.fail(f"{elf} not found; build the firmware or pass --firmware-dir")
        return elf

    return find


@pytest.fixture(autouse=True)
def show_on_display(request):
    set_status(request.node.name)
    yield
    set_status(f"{request.node.name}: done")
