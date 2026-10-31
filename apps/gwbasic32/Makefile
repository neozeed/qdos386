# Build for the GW-BASIC 32-bit port (MinGW i686 PE, runs under Wine).
#   make                 -> 64 KB workspace (original size)
#   make MEMSIZE=2097152 -> 2 MB workspace
MEMSIZE ?= 65536
AS      := i686-w64-mingw32-as -I src
CC      := i686-w64-mingw32-gcc
SRC     := $(wildcard src/*.S)
OBJ     := $(patsubst src/%.S,build/%.o,$(SRC))
MATH_OBJ := build/mathram.o build/math1.o build/math2.o
TEST_OBJ := build/math_test.o

all: build/gwbasic.exe

# The objects depend on the workspace size, so a changed MEMSIZE rebuilds them (the stamp changes only then).
build/memsize.stamp: FORCE | build
	@if [ "$$(cat $@ 2>/dev/null)" != "$(MEMSIZE)" ]; then echo $(MEMSIZE) > $@; fi
FORCE:

# .inc files are included by the sources, so they are prerequisites too
build/%.o: src/%.S build/memsize.stamp $(wildcard src/*.inc) | build
	$(AS) --defsym MEMSIZE=$(MEMSIZE) -o $@ $<

build/math_test.o: tests/math_test.S | build
	$(AS) -o $@ $<

build/gwbasic.exe: $(OBJ)
	$(CC) -nostdlib -e _mainCRTStartup -o $@ $(OBJ) -lkernel32

build:
	mkdir -p build

mathtest: build/mathtest.exe
	WINEDEBUG=-all timeout 120 wine build/mathtest.exe 2>/dev/null | python3 tests/check_math.py

build/math_cases.o: build/math_cases.S | build
	$(AS) -o $@ $<

build/math_cases.S: tests/gen_cases.py | build
	python3 tests/gen_cases.py $@

build/mathtest.exe: $(MATH_OBJ) $(TEST_OBJ) build/math_cases.o
	$(CC) -nostdlib -e _mainCRTStartup -o $@ $(MATH_OBJ) $(TEST_OBJ) build/math_cases.o -lkernel32

test: all
	python3 tests/run_gw.py

clean:
	rm -rf build

.PHONY: all clean test mathtest
