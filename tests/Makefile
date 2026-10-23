# Use `make test` at the top level

XILINX_ROOT := $(realpath $(XILINX_VIVADO)/../..)

# The container builds into build-docker/ on the host, mounted over build/, so its binaries never
# mix with a native build (the hello test builds its own, into hello/build).
ROOT := $(realpath $(CURDIR)/..)
RUN := docker run --rm -u $(shell id -u):$(shell id -g) --network=host \
	-e XILINX_VIVADO \
	-e HOME=/tmp \
	-e PATH=$(XILINX_VIVADO)/bin:/usr/local/bin:/usr/bin:/bin \
	-v $(ROOT):/work \
	-v $(ROOT)/build-docker:/work/build \
	-v $(XILINX_ROOT):$(XILINX_ROOT) \
	-v $(HOME)/.Xilinx:/tmp/.Xilinx:ro \
	-w /work

.PHONY: image test shell
image:
	docker build -t nvcx-ci -f ../Dockerfile ..

test: image
	@test -n "$(XILINX_VIVADO)" || { echo "XILINX_VIVADO is not set"; exit 1; }
	mkdir -p $(ROOT)/build-docker $(ROOT)/build # or Docker creates it root-owned
	$(RUN) nvcx-ci make pytest

shell: image
	$(RUN) -it nvcx-ci bash
