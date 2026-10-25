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

## Testing

Unfortunately there isn't an obvious public GitHub image containing Vivado
builds, so testing is currently done outside the conventional GitHub CI/CD
infrastructure.

Tested on a Ubuntu image against:

- Vivado 2025.1
- Vivado 2025.2
- Vivado 2026.1

Versions older than this have a different shared-library naming style
(`libxv_*` vs `librdi_*`) and are not supported by nvcx (although this could
be added if anyone wants it.)

## Install

You need:

* NVC 1.23, built with `--enable-tcl` and the patches in `contrib/` (these are
  submitted upstream; the `Dockerfile` shows the recipe),
* CMake 3.24 or later
* a C++20 compiler,
* Boost.Context, and
* Vivado at run time, located via `XILINX_VIVADO`.

Then:

    $ cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
    $ cmake --build build -j
    $ sudo cmake --install build

The installation includes a pkg-config file, which tells your build where each
of them went:

    $ nvc --work=nvcx:nvcx -a $(pkg-config --variable=vhdl nvcx)
    $ TCLLIBPATH=$(pkg-config --variable=tcldir nvcx) nvc ... --do script.tcl
    $ nvc ... -r --load $(pkg-config --variable=plugin nvcx) top

## Build and test

The test suite is Dockerized and is therefore fairly portable:

    $ make
    $ source /opt/xilinx/2026.1/Vivado/settings64.sh
    $ make test
    [...eventually...]
    ..................                                               [100%]
    18 passed in 115.76s (0:01:55)

(Aside: the use of Docker here also allows us to test against multiple Vivado
versions, which are unlikely to all be happy in your host OS image.)

In order to test against all suitable Vivado installations, run:

    $ make test-all  # add VIVADOES="2025.1 2025.2 2026.1" to specify

## FAQ

**Q: What versions of Vivado are supported?**
> Vivado 2025.1, 2025.2, and 2026.1 are tested via `make test-all`.
> Older versions of Vivado named their shared libraries differently (`libxv_*`
> vs `librdi_*`) and would take a little extra effort.

**Q: For XPM, should I use `nvcx` or `xpm_vhdl` (`nvc --install xpm_vhdl`)?**
> Your call. `nvcx` will be slower; `xpm_vhdl` has minor coverage gaps for
> things like ECC/error injection.  Which strategy is better is mostly a
> question of designer's preference: do you trust nvcx's simulator bridge, or
> xpm_vhdl's faithful reinterpretation of xpm RTL?

**Q: How much slower is nvcx than NVC? than xsim?**
> With a single seam, nvcx performs approximately as quickly as you'd expect
> (faster than xsim alone, slower than NVC alone, with a small extra overhead
> for the seam.) With many nvcx seams, the overhead associated with nvcx
> increases.
>
> A completely arbitrary benchmark (`make bench`) shows:
>
> ```
> mode    run [s]   uart0.txt
> nvc         5.47   same as nvc
> xsim       10.20   same as nvc
> mixed       8.92   same as nvc
> ```
>
> This includes start-up time; depending on your perspective, this may be totally
> appropriate or may penalize xsim slightly for being slow off the marks.

**Q: Is nvcx affiliated with NVC at all?**
> No! NVC is an excellent project and I should probably have chosen another name.

**Q: Is nvcx a triumph or an abomination?**
> Yes.
