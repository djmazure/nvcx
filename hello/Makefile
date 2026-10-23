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

XILINX_ROOT := $(realpath $(XILINX_VIVADO)/../..)

.PHONY: all image clean
all image clean sim snapshot bridge:
	docker build -t nvcx-hello -f ../Dockerfile ..
	docker run --rm --network=host \
		-u $(shell id -u):$(shell id -g) \
		-v $(realpath ..):/work -w /work/hello \
		-v $(XILINX_ROOT):$(XILINX_ROOT):ro \
		-v $(HOME)/.Xilinx:/tmp/.Xilinx:ro -e XILINX_VIVADO -e HOME=/tmp \
		-e PATH=$(XILINX_VIVADO)/bin:/usr/local/bin:/usr/bin:/bin \
		nvcx-hello \
		make $@

else
#
# Inside Docker: do the thing
#

.PHONY: all bridge snapshot sim clean
all: sim

# 1. Build nvcx
bridge:
	cmake -S .. -B build
	cmake --build build -j

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
	$(NVC) --work=nvcx:nvcx -a build/nvcx.vhd
	$(NVC) --work=xpm:xpm -a $(XPM)
	$(NVC) -a tb.vhd -e tb # first pass: elaborate to identify XSI-shaped holes
	./generate_shells.tcl # xelab (for xsim) and generate RTL wrappers (for nvc)
	$(NVC) -a *_shell.vhd -e tb # second pass: elaborate everything
	$(NVC) -r --load build/libnvcx.so --wave=tb.fst tb

clean:
	rm -rf vivado export_sim \
		*_shell.vhd \
		work nvcx xpm \
		tb.fst \
		xsim.wdb \
		nvcx.work \
		build \
		.Xil \
		xsim.dir \
		*.log *.jou *.pb
endif
