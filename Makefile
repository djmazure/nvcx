# This is a wrapper for Makefile dinosaurs (...me). Integrate using CMake
# instead.

.PHONY: all configure pytest test hello bench shell format clean

all: configure
	cmake --build build -j

configure build/CMakeCache.txt:
	cmake -S . -B build

# The suite itself: what `make test` runs inside the container.
pytest: all
	NVCX_BUILD=$(CURDIR)/build python3 -m pytest -v -n auto tests

hello:
	$(MAKE) -C hello

bench:
	$(MAKE) -C bench/neorv32 bench

test shell:
	$(MAKE) -C tests $@

format:
	clang-format-21 -i `git ls-files "*.cpp" "*.hpp"`
	black `git ls-files "*.py"`

clean:
	rm -rf build build-docker
