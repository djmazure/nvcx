"""Delta-cycle ordering across the boundary, xsim to NVC.

Each testbench passes in a unified simulator (NVC simulating the core itself
is the oracle) and depends on the order in which the core's outputs change
within one time step. The bridge must reproduce that order.
"""

import pytest

from test_nvc import Case, Core, run_case, run_native

CASES = {
    # the core's own clock and a counter registered on it: a register outside
    # the core, clocked by that clock, must sample the counter before its update
    "clock_then_data": Case(
        [Core("clkgen", "clkgen")], "tb_clkgen.vhd", "tb_clkgen", "120ns"
    ),
    # a combinational output derived from a registered one changes one delta
    # after it, not with it
    "comb_after_reg": Case(
        [Core("regcomb", "regcomb")], "tb_regcomb.vhd", "tb_regcomb", "120ns"
    ),
    # a zero-time pulse on an output is an event outside the core too
    "glitch": Case([Core("glitch", "glitch")], "tb_glitch.vhd", "tb_glitch", "100ns"),
}


@pytest.mark.parametrize("name", CASES)
def test_native(name, nvc, tmp_path):
    run_native(CASES[name], nvc, tmp_path)


@pytest.mark.xfail(
    strict=True,
    reason="the bridge runs the core to quiescence and deposits all its outputs in one delta",
)
@pytest.mark.parametrize("name", CASES)
def test_bridge(name, nvc, build, bridge_env, designs, tmp_path):
    run_case(CASES[name], nvc, build, bridge_env, designs, tmp_path)
