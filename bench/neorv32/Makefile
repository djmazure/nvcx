# NEORV32 (a RISC-V processor in plain VHDL) running its bootloader image under
# three scenarios:
# - all in NVC,
# - all in xsim, and
# - the processor in xsim behind an nvcx shell with the testbench in NVC.
#
#   $ source /opt/xilinx/2026.1/Vivado/settings64.sh
#   $ make bench
#
# Like hello/, runs inside the project's Docker image.

ifeq ($(XILINX_VIVADO),)
  $(error XILINX_VIVADO is not set)
endif

NEORV32_VERSION := 1.13.7
NEORV32 := neorv32-$(NEORV32_VERSION)

ifeq ($(wildcard /.dockerenv),)
#
# Outside Docker: build the image and re-run this Makefile inside it
#

DOCKER_TARGETS := bench nvc xsim mixed fetch clean
include ../../docker.mk

else
#
# Inside Docker: do the thing
#

CORE := $(shell sed 's|$$NEORV32_HOME|$(NEORV32)|' $(NEORV32)/rtl/file_list_core.f 2>/dev/null)
NVC  := nvc --std=2008
# wall-clock seconds of a command
TIME  = t0=$$(date +%s.%N); $(1); awk "BEGIN { printf \"%.2f\", $$(date +%s.%N) - $$t0 }" > $(2)

.PHONY: bench nvc xsim mixed fetch bridge clean

bench: nvc xsim mixed
	@echo
	@echo "mode    run [s]   uart0.txt"
	@for m in nvc xsim mixed; do \
		if cmp -s out/nvc/uart0.txt out/$$m/uart0.txt; then same="same as nvc"; else same="DIFFERS"; fi; \
		printf '%-7s %8s   %s\n' $$m $$(cat out/$$m/time) "$$same"; \
	done
	@echo
	@echo "xsim: $$(sed -n 's/.*Simulation CPU Usage: \([0-9]*\) ms.*/\1/p' out/xsim/run.log) ms of that is the simulation itself; the rest is xsim starting up"
	@echo
	@cat out/nvc/uart0.txt

fetch $(NEORV32)/rtl/file_list_core.f:
	wget -q -O - https://github.com/stnolting/neorv32/archive/refs/tags/v$(NEORV32_VERSION).tar.gz \
		| tar xz --wildcards '$(NEORV32)/rtl/core/*' '$(NEORV32)/rtl/file_list_core.f'

# nvcx, installed into a local prefix (as in hello/)
PREFIX := $(CURDIR)/prefix
PKG_CONFIG := PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig pkg-config
bridge:
	cmake -S ../.. -B build -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	cmake --build build -j
	cmake --install build

# Where the installed pieces are (deferred: they exist only after `bridge`)
NVCX_PLUGIN = $(shell $(PKG_CONFIG) --variable=plugin nvcx)
NVCX_VHDL   = $(shell $(PKG_CONFIG) --variable=vhdl nvcx)
NVCX_TCLDIR = $(shell $(PKG_CONFIG) --variable=tcldir nvcx)

# 1. NVC simulates everything
nvc: $(NEORV32)/rtl/file_list_core.f
	rm -rf out/nvc && mkdir -p out/nvc
	$(NVC) --work=neorv32:out/nvc/neorv32 -a $(CORE)
	$(NVC) --map=neorv32:out/nvc/neorv32 --work=work:out/nvc/work -a bench_pkg.vhd soc.vhd tb.vhd -e tb
	cd out/nvc && $(call TIME,$(NVC) --map=neorv32:neorv32 --work=work:work -r --ieee-warnings=off --stop-time=1sec tb,time)

# 2. xsim simulates everything
xsim: $(NEORV32)/rtl/file_list_core.f
	rm -rf out/xsim && mkdir -p out/xsim
	cd out/xsim && xvhdl --2008 --work neorv32 $(addprefix ../../,$(CORE)) > /dev/null
	cd out/xsim && xvhdl --2008 ../../bench_pkg.vhd ../../soc.vhd ../../tb.vhd > /dev/null
	cd out/xsim && xelab --log xelab-tb.log --timeprecision_vhdl 1fs tb -s tb > /dev/null
	cd out/xsim && xelab --log xelab-soc.log --timeprecision_vhdl 1fs -dll soc --snapshot soc > /dev/null
	cd out/xsim && $(call TIME,xsim --log run.log tb -R > /dev/null,time)

# 3. the processor in xsim, the testbench in NVC
mixed: bridge xsim
	rm -rf out/mixed && mkdir -p out/mixed
	$(NVC) --work=nvcx:out/mixed/nvcx -a $(NVCX_VHDL)
	$(NVC) --map=nvcx:out/mixed/nvcx --work=work:out/mixed/work -a bench_pkg.vhd tb.vhd -e tb
	TCLLIBPATH=$(NVCX_TCLDIR) ./nvc.tcl
	$(NVC) --map=nvcx:out/mixed/nvcx --work=work:out/mixed/work -a out/mixed/soc_shell.vhd -e tb
	cd out/mixed && $(call TIME,$(NVC) --map=nvcx:nvcx --work=work:work -r --ieee-warnings=off --load $(NVCX_PLUGIN) --stop-time=1sec tb,time)

clean:
	rm -rf out build prefix nvcx.work xsim.dir *.log *.jou *.pb
endif
