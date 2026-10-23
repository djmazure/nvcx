#!/usr/bin/env -S TCLLIBPATH=build/tcl nvc --std=2008 --map=nvcx:out/mixed/nvcx --work=work:out/mixed/work --do tb
package require nvcx
nvcx::write_shell -component soc -output out/mixed/soc_shell.vhd -snapshot out/xsim/xsim.dir/soc
