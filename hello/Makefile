# Hello, world: an encrypted Xilinx DDS core simulated by xsim, inside an NVC
# simulation.
#
#   $ source /opt/xilinx/2026.1/Vivado/settings64.sh  # or whatever
#   $ make
#
# Uses Docker (with Vivado bind-mounted) to avoid too much dependence on the
# host OS/runtime.

ifeq ($(XILINX_VIVADO),)
  $(error XILINX_VIVADO is not set)
endif

ifeq ($(wildcard /.dockerenv),)
#
# Outside Docker: build the image and re-run this Makefile inside it
#

DOCKER_TARGETS := all clean sim snapshot bridge
include ../docker.mk

else
#
# Inside Docker: do the thing
#

.PHONY: all bridge snapshot sim clean
all: sim

# 1. Build and install nvcx into a local prefix. To use an nvcx that's already
#    installed, drop `bridge` from sim's prerequisites below: pkg-config falls
#    back to its default search path.
PREFIX := $(CURDIR)/prefix
PKG_CONFIG := PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig pkg-config
bridge:
	cmake -S .. -B build -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	cmake --build build -j
	cmake --install build

# Where the installed pieces are (deferred: they exist only after `bridge`)
NVCX_PLUGIN = $(shell $(PKG_CONFIG) --variable=plugin nvcx)
NVCX_VHDL   = $(shell $(PKG_CONFIG) --variable=vhdl nvcx)
NVCX_TCLDIR = $(shell $(PKG_CONFIG) --variable=tcldir nvcx)

# 2. Vivado
EXPORT   := export_sim/hello_dds/xsim
SNAPSHOT := $(EXPORT)/xsim.dir/hello_dds/xsimk.so
snapshot: $(SNAPSHOT)
$(SNAPSHOT): generate_ip.tcl
	./generate_ip.tcl
	sed -i '/^  xelab -dll /! s/^  xelab /  xelab -dll /' $(EXPORT)/hello_dds.sh
	cd $(EXPORT) && ./hello_dds.sh -step compile
	cd $(EXPORT) && ./hello_dds.sh -step elaborate

# 3. NVC
XPM := $(XILINX_VIVADO)/data/ip/xpm/xpm_VCOMP.vhd
NVC := nvc --std=2008 --map=nvcx:nvcx --map=xpm:xpm
sim: bridge $(SNAPSHOT)
	# Teach your preferred build tool how to do it better
	rm -rf work
	$(NVC) --work=nvcx:nvcx -a $(NVCX_VHDL)
	$(NVC) --work=xpm:xpm -a $(XPM)
	$(NVC) -a tb.vhd -e tb # first pass: elaborate to identify XSI-shaped holes
	TCLLIBPATH=$(NVCX_TCLDIR) ./generate_shells.tcl # xelab (for xsim) and generate RTL wrappers (for nvc)
	$(NVC) -a *_shell.vhd -e tb # second pass: elaborate everything
	$(NVC) -r --load $(NVCX_PLUGIN) --wave=tb.fst tb

clean:
	rm -rf vivado export_sim \
		*_shell.vhd \
		work nvcx xpm \
		tb.fst \
		xsim.wdb \
		nvcx.work \
		build prefix \
		.Xil \
		xsim.dir \
		*.log *.jou *.pb
endif
