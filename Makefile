# One production target: accepted Apollo R7B2, with project-owned toolchain sources.
.DEFAULT_GOAL := all
BUILD_DIR ?= $(CURDIR)/build/current
HOSTCC ?= gcc
PAYLOAD_IMAGE ?=
.PHONY: all production apollo-r7-rw toolchain sdk kernel shell disk-tools image clean help
all production apollo-r7-rw: image
toolchain sdk kernel shell disk-tools image:
	python3 tools/build.py $@ --output "$(BUILD_DIR)" --hostcc "$(HOSTCC)" $(if $(PAYLOAD_IMAGE),--payload "$(PAYLOAD_IMAGE)")
clean:
	python3 tools/build.py clean --output "$(BUILD_DIR)"
help:
	@printf '%s\n' 'make: build historical tools, SDK, R7B2 kernel/shell, utilities and bootable disk' \
	 'BUILD_DIR=/path/to/new/output selects the single generated-output tree' \
	 'PAYLOAD_IMAGE=/path/to/accepted-fat16.img adds preserved private applications' \
	 'make clean removes only a marked build tree; historical profiles remain at frozen tags'
