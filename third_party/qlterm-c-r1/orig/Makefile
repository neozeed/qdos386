# QDOS GCC 2.95.3 / JAS / LD. No modern assembler required.
QDOS_CC ?= /usr/local/qdos/bin/qdos-gcc
CFLAGS ?= -O2 -Wall -W -fno-builtin
SOURCES = src/main.c src/platform.c src/screen.c src/serio.c src/filexfer.c src/font.c src/config.c
OBJECTS = $(SOURCES:src/%.c=build/%.o)
all: qlterm
build:
	mkdir -p build
build/%.o: src/%.c src/qlterm.h src/config.h src/crc_table.h | build
	$(QDOS_CC) $(CFLAGS) -c $< -o $@
qlterm: $(OBJECTS)
	$(QDOS_CC) $(OBJECTS) -o $@
data:
	python3 tools/generate_data.py
clean:
	rm -f build/*.o qlterm
.PHONY: all clean data
