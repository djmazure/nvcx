# Use `make test` at the top level

# The container builds into build-docker/ on the host, mounted over build/, so its binaries never
# mix with a native build (the hello test builds its own, into hello/build).
DOCKER_WORKDIR := /work
include ../docker.mk
DOCKER_ARGS = -v $(NVCX_ROOT)/build-docker:/work/build

.PHONY: image test test-all shell
image:
	$(DOCKER_BUILD)
	mkdir -p $(NVCX_ROOT)/build-docker $(NVCX_ROOT)/build # or Docker creates them root-owned

test: image
	$(DOCKER_RUN) make pytest

# All "blessed" Vivado releases
VIVADOES ?= 2025.1 2025.2 2026.1
test-all:
	@fail=; for v in $(VIVADOES); do \
		echo "=== $$v"; \
		XILINX_VIVADO=$(XILINX_ROOT)/$$v/Vivado $(MAKE) --no-print-directory test || fail="$$fail $$v"; \
	done; \
	test -z "$$fail" || { echo "FAILED:$$fail"; exit 1; }

shell: DOCKER_ARGS += -it
shell: image
	$(DOCKER_RUN) bash
