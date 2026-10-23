import shutil
import subprocess
from dataclasses import dataclass, field
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
TB = ROOT / "tests" / "tb"


@dataclass
class Core:
    name: str
    design: str
    library: str | None = None


@dataclass
class Case:
    cores: list[Core]
    tb: str  # testbench file in tests/tb
    top: str
    stop: str
    sources: list[str] = field(
        default_factory=list
    )  # extra tests/tb files to analyse first
    run_args: list[str] = field(default_factory=list)
    check: object = None  # check(tmp_path, result) after a passing run
    expect_failure: list[str] | None = (
        None  # the run must stop, with these texts in its output
    )
    tcl_expect: list[str] = field(default_factory=list)  # texts nvcx.tcl must print
    before_run: object = (
        None  # before_run(tmp_path): tamper between nvcx.tcl and the run
    )


def ports_in_vcd(tmp_path, r):
    """The shell's ports are ordinary NVC signals: they appear in NVC's own waveform output."""
    text = (tmp_path / "nvc.vcd").read_text()
    assert "cnt_out" in text and "y_comb" in text, text[:2000]


def two_snapshots_then_cached(tmp_path, r):
    snapshots = sorted(p.name for p in (tmp_path / "work" / "snapshots").iterdir())
    assert len(snapshots) == 2 and all(
        s.startswith("dut-") for s in snapshots
    ), snapshots
    r.rerun()


def drop_width_12(tmp_path):
    """Remove the WIDTH=12 snapshot after nvcx.tcl made it: the plugin must not elaborate."""
    for key in (tmp_path / "work" / "snapshots").glob("*/key.txt"):
        if "WIDTH=12" in key.read_text():
            shutil.rmtree(key.parent)


CASES = {
    # combinational and registered paths, and the core's own oscillator, during activity and idle
    "exp": Case([Core("exp", "exp")], "tb_exp.vhd", "tb_exp", "250ns"),
    # two instances of one snapshot and one of another: three private kernels in one NVC process
    "multi": Case(
        [Core("dut", "dut"), Core("exp", "exp")], "tb_multi.vhd", "tb_multi", "100ns"
    ),
    # a Verilog-top snapshot behind the testbench's own component declaration of it, 4-state
    # values crossing both ways
    "verilog": Case([Core("dut_v", "dut_v")], "tb_vlog.vhd", "tb_vlog", "100ns"),
    # integer, real and boolean ports cross the boundary with their values intact
    "scalars": Case([Core("hier", "hier")], "tb_hier.vhd", "tb_hier", "10ns"),
    # the shell's ports are NVC signals: present in NVC's waveform output
    "waves": Case(
        [Core("exp", "exp")],
        "tb_exp.vhd",
        "tb_exp",
        "250ns",
        run_args=["--wave=nvc.vcd", "--format=vcd"],
        check=ports_in_vcd,
    ),
    # a component from a package, instantiated with two generic sets: one shell, one snapshot per set
    "generic": Case(
        [Core("dut", "dut", library="work")],
        "tb_generic.vhd",
        "tb_generic",
        "100ns",
        sources=["dut_comp.vhd"],
        check=two_snapshots_then_cached,
        tcl_expect=[
            "elaborating work.dut for :TB_GENERIC:NARROW",
            "elaborating work.dut for :TB_GENERIC:WIDE",
        ],
    ),
    # the plugin never elaborates: a snapshot gone missing is a warning naming the script to
    # re-run, and its instance a black box, so the testbench's own check of that instance fails
    "generic_missing": Case(
        [Core("dut", "dut", library="work")],
        "tb_generic.vhd",
        "tb_generic",
        "100ns",
        sources=["dut_comp.vhd"],
        before_run=drop_width_12,
        expect_failure=["WIDTH=12", "is missing", "re-run", "black box"],
    ),
    # a report of severity failure inside the core arrives as an NVC assertion failure and ends
    # the run; the testbench's own "FAIL" report must never be reached
    "doom": Case(
        [Core("dut", "dut")],
        "tb_doom.vhd",
        "tb_doom",
        "100ns",
        expect_failure=["** Failure:", "DOOM!"],
    ),
}


@pytest.mark.parametrize("name", CASES)
def test_nvc_case(name, nvc, build, bridge_env, designs, tmp_path):
    from conftest import DESIGNS, RTL

    case = CASES[name]
    env = dict(bridge_env, NVCX_WORK=str(tmp_path / "work"))
    env["PATH"] = f"{Path(env['XILINX_VIVADO']) / 'bin'}:{env['PATH']}"

    # the design's NVC-side script: what xsim provides for each core
    calls = ["package require nvcx"]
    for c in case.cores:
        if c.library is None:
            calls.append(
                f"nvcx::write_shell -component {c.name} -output {tmp_path / c.name}_shell.vhd"
                f" -snapshot {designs(c.design)}"
            )
        else:
            lang, source, _ = DESIGNS[c.design]
            prj = tmp_path / f"{c.design}.prj"
            prj.write_text(
                f"{'vhdl2008' if lang == 'vhdl' else 'sv'} {c.library} {RTL / source}\n"
            )
            calls.append(
                f"nvcx::write_shell -component {c.name} -output {tmp_path / c.name}_shell.vhd"
                f" -library {c.library} -elab {{-prj {prj}}}"
            )
    script = tmp_path / "nvc.tcl"
    script.write_text("\n".join(calls) + "\n")
    env["TCLLIBPATH"] = str(build / "tcl")

    # the build tree keeps everything side by side (an install spreads it over lib/ and share/)
    core = str(build / "nvcx.vhd")
    plugin = str(build / "libnvcx.so")
    subprocess.run(
        [nvc, "--std=2008", f"--work=nvcx:{tmp_path / 'nvcx'}", "-a", core],
        check=True,
        cwd=tmp_path,
    )
    common = [
        nvc,
        "--std=2008",
        f"--map=nvcx:{tmp_path / 'nvcx'}",
        f"--work=work:{tmp_path / 'work_lib'}",
    ]

    def nvc_run(*args, **kw):
        return subprocess.run(
            [*common, *args],
            cwd=tmp_path,
            env=env,
            capture_output=True,
            text=True,
            **kw,
        )

    # pass 1: the cores unbound
    r = nvc_run("-a", *(str(TB / f) for f in case.sources), str(TB / case.tb))
    assert r.returncode == 0, r.stdout + r.stderr
    r = nvc_run("-e", case.top)
    assert r.returncode == 0, r.stdout + r.stderr
    r = nvc_run("--do", case.top, str(script))
    assert r.returncode == 0, r.stdout + r.stderr
    for text in case.tcl_expect:
        assert text in r.stdout, (text, r.stdout + r.stderr)
    shells = sorted(tmp_path.glob("*_shell.vhd"))
    assert {s.name for s in shells} == {
        f"{c.name}_shell.vhd" for c in case.cores
    }, shells
    if case.before_run:
        case.before_run(tmp_path)
    # pass 2: the shells provide the entities
    r = nvc_run("-a", *map(str, shells))
    assert r.returncode == 0, r.stdout + r.stderr
    r = nvc_run("-e", case.top)
    assert r.returncode == 0, r.stdout + r.stderr

    def run():
        r = nvc_run(
            "-r", "--load", plugin, f"--stop-time={case.stop}", *case.run_args, case.top
        )
        out = r.stdout + r.stderr
        if case.expect_failure:
            # NVC's own failure exit, not a crash on the way out
            assert r.returncode == 1 and "Caught signal" not in out, out
            for text in case.expect_failure:
                assert text in out, (text, out)
            return r
        assert r.returncode == 0 and "PASS" in out, out
        r.rerun = run
        return r

    r = run()
    if case.check:
        case.check(tmp_path, r)
