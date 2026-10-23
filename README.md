# nvcx

Embed AMD/Xilinx xsim inside [NVC](https://www.nickg.me.uk/nvc/). This is
primarily intended to allow encrypted IP and SystemVerilog with generics to be
simulated in an NVC "world", removing a major feature gap in open-source RTL
simulation.

Cores delegated to xsim become a VHDL shell and a VHPI back-end containing
xsim's simulation model. Because NVC simulates most of the design, non-xsim
parts of the world (ideally, most of it) can freely use VHPI, PSL, FST/VDB, et
cetera. xsim only exists to plug encrypted-IP and SystemVerilog gaps in NVC.

If you're curious, try "hello":

    $ cd hello
    $ ls
    generate_ip.tcl  generate_shells.tcl  Makefile  README.md  tb.vhd
    $ source /opt/xilinx/2026.1/Vivado/settings64.sh
    $ make
    [...eventually...]
    ** Note: 7825ns+0: PASS: 256
       Process :tb:check at tb.vhd:70
    ** Note: 7825ns+0: FINISH called
       Procedure FINISH [] at lib/std.08/env-body.vhd:42
       Process :tb:check at tb.vhd:126

## Why?

* NVC can now (with help) simulate encrypted IP - this has been a longstanding
  and fundamental issue for open-source RTL simulators, which (for obvious and
  fundamental reasons) can't make use of IEEE 1735-style encrypted IP.

* xsim is now a "polyfill" for NVC - a SystemVerilog design that's beyond NVC's
  current capabilities (e.g. AMD/Xilinx xpm, which use entity-level generics)
  can now instantiate these IP blocks in xsi and simulate them as black boxes.

* Similarly, NVC can be a "polyfill" for xsim - when using NVC as a top-level
  simulation kernel and xsim for the "mop-up" terminal IPs, I can make use of
  VHPI, PSL, FST/VCD, cocotb, et cetera.

## Build and test

Building needs CMake and a C++20 compiler. Running needs Vivado
(`XILINX_VIVADO`) set; the simulator kernel and its libraries are loaded from
the installation in place) and NVC built with `--enable-tcl` and the patches in
contrib/ applied.  Python is needed for the test suite.

`make install` puts the plugin `libnvcx.so` and the bridge stub in `lib/nvcx/`,
and `nvcx.vhd` with the Tcl scripts in `share/nvcx/`; a build tree has all of
them side by side in `build/`.

    $ make
    $ source /opt/xilinx/2026.1/Vivado/settings64.sh
    $ make test
    [...eventually...]
    ..................                                               [100%]
    18 passed in 115.76s (0:01:55)

## Disclaimer

This is just a proof-of-concept. It has not been tested "in anger". Comments
very welcome at <gsmecher@threespeedlogic.com>.

* Is nvcx affiliated with NVC at all? No.
* Is nvcx a triumph or an abomination? Yes.
