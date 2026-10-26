import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_hello(nvc, vivado, tmp_path):
    for name in [
        "CMakeLists.txt",
        "nvcx.pc.in",
        "src",
        "include",
        "man",
        "vhdl",
        "tcl",
        "tests",
        "hello",
    ]:
        src = ROOT / name
        if src.is_dir():
            shutil.copytree(
                src,
                tmp_path / name,
                ignore=shutil.ignore_patterns(
                    "build", "__pycache__", "nvcx.work", "work"
                ),
            )
        else:
            shutil.copy(src, tmp_path / name)
    env = dict(
        os.environ,
        PATH=os.pathsep.join(
            [str(Path(nvc).parent), str(vivado / "bin"), os.environ["PATH"]]
        ),
    )
    cmd = ["make", "-C", str(tmp_path / "hello"), "clean", "all"]
    r = subprocess.run(cmd, env=env, capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    assert "PASS: 256" in r.stdout + r.stderr, (
        r.stdout[-3000:] + r.stderr[-3000:]
    )  # NVC reports on stderr
