#!/usr/bin/env -S nvc --std=2008 --map=nvcx:nvcx --map=xpm:xpm --do tb

package require nvcx

nvcx::write_shell -component hello_dds \
	-output hello_dds_shell.vhd \
	-snapshot export_sim/hello_dds/xsim/xsim.dir/hello_dds

nvcx::write_shell -component xpm_memory_sdpram \
	-output xpm_memory_sdpram_shell.vhd \
	-library xpm
