# libqdos.a: the QDOS/386 SDK wrappers and the trap entry.  Built with the
# historical tree, as qdos386-gcc does.  Requires QDOS386_TOOLS.
ifndef QDOS386_TOOLS
$(error set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree)
endif
GCC = $(QDOS386_TOOLS)/bin/i386-gcc
AS  = $(QDOS386_TOOLS)/bin/as
AR  = ar
SDK = $(abspath ..)

libqdos.a: qdos.o qtrap.o qdospg.o
	rm -f $@
	$(AR) rc $@ qdos.o qtrap.o qdospg.o

qdos.o: qdos.c ../include/qdos386.h
	$(GCC) -c -O -I../include qdos.c -o $@

# R11K: paging entry points, a separate member so programs that do not call them are unchanged.
qdospg.o: qdospg.c ../include/qdos386.h
	$(GCC) -c -O -I../include qdospg.c -o $@

qtrap.o: qtrap.s
	$(AS) -o $@ qtrap.s

crt0.o: crt0.s
	$(AS) -o $@ crt0.s

clean:
	rm -f *.o libqdos.a

.PHONY: clean
