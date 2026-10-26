"""The engine's in-process tests (tests/engine_tests.cpp), one pytest case each."""

import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]


def _names():
    exe = ROOT / "build" / "nvcx-tests"
    if not exe.is_file():
        return ["(not built)"]
    return subprocess.run(
        [exe, "--list"], capture_output=True, text=True, check=True
    ).stdout.split()


@pytest.mark.parametrize("name", _names())
def test_engine(name, build, designs):
    if name == "(not built)":
        pytest.skip("nvcx-tests not built")
    r = subprocess.run(
        [build / "nvcx-tests", designs.all(), name],
        capture_output=True,
        text=True,
    )
    assert r.returncode == 0 and f"PASS {name}" in r.stdout, r.stdout + r.stderr
