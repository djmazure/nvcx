"""Regressions: one case per fixed bug, each the smallest design that showed it. A case's comment
says what went wrong, so that a failure here names the bug that came back."""

import pytest

from test_nvc import Case, Core, run_case

REGRESSIONS = {
    # nvcx.tcl passed string generics to xelab as -generic_top NAME=VALUE, unquoted, so xelab took
    # a numeric-looking one ("0") as a number. XPM's READ_RESET_VALUE_A => "0" arrived as the
    # integer 0, and its DRC called that non-zero.
    "string_generic": Case(
        [Core("strgen", "strgen", library="work")],
        "tb_strgen.vhd",
        "tb_strgen",
        "10ns",
        sources=["strgen_comp.vhd"],
    ),
    "string_generic_v": Case(
        [Core("strgen", "strgen_v", library="work")],
        "tb_strgen.vhd",
        "tb_strgen",
        "10ns",
        sources=["strgen_comp.vhd"],
    ),
}


@pytest.mark.parametrize("name", REGRESSIONS)
def test_regression(name, nvc, build, bridge_env, designs, tmp_path):
    run_case(REGRESSIONS[name], nvc, build, bridge_env, designs, tmp_path)
