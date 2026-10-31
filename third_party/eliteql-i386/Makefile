# Native 80386 gameplay and shell assembly, with the KCTX-FIX2 adapter.
AS138 ?= build/tools/as138
CC ?= gcc
LD ?= ld
PYTHON ?= python3
MODULES = save keyboard  universe  explosion dust dust_generated  render_game planet_view  presentation sound dials scanner dashboard models_game release  screen_store bulbs  controls commander universe_list views random equipment cargo station_ai motion shop computer_dock travel flight tidy line combat tactics navigation missile docking target market planet station encounters compass
TESTS = universe  explosion dust  planet_view  presentation fidelity dials scanner  compass encounters_pool  controls commander universe_list views equipment cargo motion shop fittings travel extra flight tidy combat tactics navigation missile docking target market planet station encounters
OBJECTS = $(MODULES:%=build/aout/%.o)
.PHONY: all objects check clean
all: objects
objects: $(OBJECTS)
build/tools/as138: tools/build-gas138.sh $(wildcard third_party/gas-1.38/*)
	sh tools/build-gas138.sh
build/aout/%.o: src/%.s $(AS138)
	mkdir -p build/aout
	$(AS138) -o $@ $<
	$(PYTHON) tools/check-aout-symbols.py $< $@
# Host ELF is only a way to execute the exact assembly and original fixtures.
# Adapt a.out relocation records for the host linker; execute GAS 1.38 bytes.
build/elf/%.o: build/aout/%.o tools/aout-to-elf.py
	mkdir -p build/elf
	$(PYTHON) tools/aout-to-elf.py $< $@
build/elf/runtime.o: tests/host/runtime.c
	mkdir -p build/elf
	$(CC) -m32 -march=i386 -O2 -fcommon -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -Itests/host -c $< -o $@
build/elf/start.o: tests/host/start.s
	mkdir -p build/elf
	as --32 -o $@ $<
build/elf/extra_test.o: tests/extra_test.c $(wildcard reference/tests/*_fixtures.h) tests/host/stdio.h tests/host/string.h
	mkdir -p build/elf
	$(CC) -m32 -march=i386 -O2 -fcommon -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -Itests/host -Ireference/tests -c $< -o $@
build/elf/%_test.o: reference/tests/%_test.c $(wildcard reference/tests/*_fixtures.h) tests/host/stdio.h tests/host/string.h
	mkdir -p build/elf
	$(CC) -m32 -march=i386 -O2 -fcommon -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -Itests/host -c $< -o $@
build/elf/%_test: build/elf/%_test.o $(MODULES:%=build/elf/%.o) build/elf/runtime.o build/elf/platform.o build/elf/start.o
	$(LD) -m elf_i386 -o $@ $^
check: $(TESTS:%=build/elf/%_test)
	@set -e; for t in $(TESTS); do $(PYTHON) tools/run-i386.py build/elf/$$t\_test; done
clean:
	$(PYTHON) -c "import shutil; shutil.rmtree('build', ignore_errors=True)"

.SECONDARY:

build/elf/platform.o: tests/host/platform.c
	$(CC) -m32 -march=i386 -O2 -fcommon -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -c $< -o $@

LD194 = build/tools/ld194
build/tools/ld194: tools/build-ld194.sh
	sh tools/build-ld194.sh
build/aout/headless.o: tests/host/headless.s $(AS138)
	$(AS138) -o $@ $<
	$(PYTHON) tools/check-aout-symbols.py $< $@
build/aout/core-headless: $(OBJECTS) build/aout/headless.o $(LD194)
	$(LD194) -N -e _headless_start -Ttext 0x10000 -o $@ $(OBJECTS) build/aout/headless.o
.PHONY: historical-link
historical-link: build/aout/core-headless

ORACLE_MODULES = motion flight tidy line combat tactics navigation missile docking target render_game models_game planet_view dials scanner dashboard presentation screen_store explosion dust dust_generated universe travel
build/tools/as138-m68k: tools/build-gas138-m68k.sh $(wildcard third_party/gas-1.38/*)
	sh tools/build-gas138-m68k.sh
oracle/core-m68k.o: tools/build-m68k-oracle.py $(ORACLE_MODULES:%=reference/src/%.s) build/tools/as138-m68k
	$(PYTHON) tools/build-m68k-oracle.py $(ORACLE_MODULES)
	build/tools/as138-m68k -m68000 -o $@ oracle/core-m68k.s
.PHONY: oracle-check
oracle-check: oracle/core-m68k.o build/elf/flight_test historical-link
	@set -e; for t in movement raster render effects; do ELITE_NATIVE_IMAGE=build/aout/core-headless $(PYTHON) tools/diff-$$t.py; done

build/elf/platform_test.o: tests/platform_test.c tests/host/stdio.h tests/host/string.h
	$(CC) -m32 -march=i386 -O2 -fcommon -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -Itests/host -c $< -o $@
build/elf/platform_test: build/elf/platform_test.o $(MODULES:%=build/elf/%.o) build/elf/runtime.o build/elf/start.o
	$(LD) -m elf_i386 -o $@ $^
.PHONY: platform-check
platform-check: build/elf/platform_test
	$(PYTHON) tools/run-i386.py $<

.PHONY: alternatives
alternatives: build/aout/render.o build/aout/models.o build/aout/development.o

# R3: original shell leaf routines; no replacement main loop.
R3_OBJECTS = build/aout/shell_text.o build/aout/shell_save.o
build/aout/r3-shell-headless: $(OBJECTS) $(R3_OBJECTS) build/aout/headless.o $(LD194)
	$(LD194) -N -e _headless_start -Ttext 0x10000 -o $@ $(OBJECTS) $(R3_OBJECTS) build/aout/headless.o
oracle/shell-m68k.o: reference/src/shell.s tools/build-shell-oracle.py build/tools/as138-m68k
	$(PYTHON) tools/build-shell-oracle.py
	build/tools/as138-m68k -m68000 -o $@ oracle/shell-m68k.s
.PHONY: shell-check
shell-check: build/aout/r3-shell-headless oracle/shell-m68k.o oracle/core-m68k.o
	ELITE_NATIVE_IMAGE=build/aout/r3-shell-headless $(PYTHON) tools/diff-shell-text.py

# Complete R3 shell, using the same historical assembler and linker as R2.
R3_MODULES = shell_text shell_save shell_data shell_cheats shell_start shell_setup shell_scene shell_flow shell_sound shell_universe shell_adapters shell_spawn shell_world
R3_ALL_OBJECTS = $(R3_MODULES:%=build/aout/%.o)
build/aout/ELITE.AOU: ld/elite.ld $(OBJECTS) $(R3_ALL_OBJECTS) build/aout/platform_qdos.o $(LD194)
	$(LD194) -N -e _start -T ld/elite.ld -o $@ build/aout/platform_qdos.o $(OBJECTS) $(R3_ALL_OBJECTS)
build/aout/r3-platform-test: $(OBJECTS) $(R3_ALL_OBJECTS) build/aout/platform_qdos.o $(LD194)
	$(LD194) -N -e _start -Ttext 0x10000 -o $@ build/aout/platform_qdos.o $(OBJECTS) $(R3_ALL_OBJECTS)
.PHONY: r3-check r3-game
r3-game: build/aout/ELITE.AOU
	$(PYTHON) tools/check-r3-image.py
r3-check: $(R3_ALL_OBJECTS) oracle/core-m68k.o build/tools/as138-m68k
	$(PYTHON) tools/build-full-shell-oracle.py
	$(PYTHON) tools/build-shell-audit.py
	$(PYTHON) tools/r3-shell-inventory.py
	@set -e; for t in setup lifecycle universe world serialization pages loop; do ELITE_NATIVE_IMAGE=build/aout/shell-audit $(PYTHON) tools/diff-shell-$$t.py; done
