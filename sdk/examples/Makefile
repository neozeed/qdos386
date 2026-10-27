# SDK examples.  Build with the historical tree:
#   QDOS386_TOOLS=/path/to/tree make -C sdk/examples
# Each example is built with bin/qdos386-gcc only (no hand-written trap code).
ifndef QDOS386_TOOLS
$(error set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree)
endif
WRAP = ../bin/qdos386-gcc
export QDOS386_TOOLS

all: SDKHEL.aout SDKARG.aout SDKSTM.aout SDKSTS.aout LIBHEL.aout CATFIL.aout MEMSTR.aout BIGAPP.aout HEAPEX.aout

# R9: the C library proofs (libc.a, see sdk/libc and docs/R9-LIBC-ABI.md).
LIBHEL.aout: libhello/libhello.c
	$(WRAP) -o $@ libhello/libhello.c

CATFIL.aout: catfil/catfil.c
	$(WRAP) -o $@ catfil/catfil.c

MEMSTR.aout: memstr/memstr.c
	$(WRAP) -o $@ memstr/memstr.c

# BIGAPP is larger than 224 KiB (the R8 limit): its table is generated.
bigapp/table.c: bigapp/gen_table.py
	python3 bigapp/gen_table.py > $@

BIGAPP.aout: bigapp/bigapp.c bigapp/table.c
	$(WRAP) -o $@ bigapp/bigapp.c bigapp/table.c

HEAPEX.aout: heapex/heapex.c
	$(WRAP) -o $@ heapex/heapex.c

SDKHEL.aout: hello/hello.c
	$(WRAP) -o $@ hello/hello.c

SDKARG.aout: args/args.c
	$(WRAP) -o $@ args/args.c

SDKSTM.aout: stream/stream.c
	$(WRAP) -o $@ stream/stream.c

SDKSTS.aout: stress/stress.c
	$(WRAP) -o $@ stress/stress.c

clean:
	rm -f *.aout bigapp/table.c

.PHONY: all clean
