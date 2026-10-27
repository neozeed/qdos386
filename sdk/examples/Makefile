# SDK examples.  Build with the historical tree:
#   QDOS386_TOOLS=/path/to/tree make -C sdk/examples
# Each example is built with bin/qdos386-gcc only (no hand-written trap code).
ifndef QDOS386_TOOLS
$(error set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree)
endif
WRAP = ../bin/qdos386-gcc
export QDOS386_TOOLS

all: SDKHEL.aout SDKARG.aout SDKSTM.aout SDKSTS.aout

SDKHEL.aout: hello/hello.c
	$(WRAP) -o $@ hello/hello.c

SDKARG.aout: args/args.c
	$(WRAP) -o $@ args/args.c

SDKSTM.aout: stream/stream.c
	$(WRAP) -o $@ stream/stream.c

SDKSTS.aout: stress/stress.c
	$(WRAP) -o $@ stress/stress.c

clean:
	rm -f *.aout

.PHONY: all clean
