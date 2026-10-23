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

## Timing

NVC and each xsim cores advance in lockstep. The plugin pauses the xsim kernel
at every delta boundary (a hook on the kernel's signal-update phase, with the
kernel running on a coroutine), writes the inputs NVC changed in its delta,
lets the kernel run one delta, and deposits the outputs that changed for NVC's
next delta. Each side sees the other's assignments one delta after they were
made, as it would in a single simulator. Deltas in which no core output changes
while NVC has nothing pending run without an NVC cycle, since nothing on NVC's
side could have happened in them.  `tests/test_delta.py` checks
delta-cycle properties against NVC simulating the same cores natively.

I suspect this is not a 100% rigorous reproduction of the delta-cycle semantics
of a single simulator kernel.  I also suspect it doesn't matter under
conventional (synchronous) design practice. I'm not a simulator expert, so you
are (unfortunately) going to have to convince yourself. If you do, regardless
of your conclusions, drop me a note.

## Build and test

The test suite is Dockerized and is therefore fairly portable:

    $ make
    $ source /opt/xilinx/2026.1/Vivado/settings64.sh
    $ make test
    [...eventually...]
    ..................                                               [100%]
    18 passed in 115.76s (0:01:55)

## Benchmark

`make bench` runs NEORV32 through its bootloader's UART console three ways:

- everything in NVC,
- everything in xsim, and
- the processor in xsim behind an nvcx shell with the testbench in NVC.

The console transcript and the cycle count must be identical in order for the
underlying simulations to be correct. Wall times are an indication of the
relative performance of each engine.

## Disclaimer

This is just a proof-of-concept. It has not been tested "in anger". Comments
very welcome at <gsmecher@threespeedlogic.com>.

* Is nvcx affiliated with NVC at all? No.
* Is nvcx a triumph or an abomination? Yes.
