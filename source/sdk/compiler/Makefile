# libgcc-qd.a: the compiler support routines that GCC 1.40 calls for i386
# code and that the SDK did not provide (R11C).  Each routine is compiled
# from the historical GCC 1.40 gnulib sources with its L_ macro, into its own
# object, so that the linker pulls in only what a program references.
#
#   usage: QDOS386_TOOLS=<historical tree> make -C sdk/compiler
#
# The historical tree must contain gcc-1.40/gnulib.c and gcc-1.40/gnulib2.c.
# Those files are CRLF; they are converted to LF in a work directory and
# are never modified.  Output: ../lib/libgcc-qd.a.
ifndef QDOS386_TOOLS
$(error set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree)
endif
GCC = $(QDOS386_TOOLS)/bin/i386-gcc
AR  = ar
AS  = $(QDOS386_TOOLS)/bin/as
SDKINC = ../include
GNUSRC = $(QDOS386_TOOLS)/gcc-1.40

# fixdfsi.o is NOT from gnulib.  GCC 1.40 compiles the C body "(SItype) a"
# of gnulib's __fixdfsi into a call to ___fixdfsi itself, so that routine
# recurses without end (found in R11C).  It is replaced by the x87 routine in
# fixdfsi.s (emx/gcc, see docs/R11C-LICENSE-PROVENANCE.md).
#
# gnulib2.c: 64-bit conversions.  fixunsdfdi is needed by fixdfdi (and it uses
# adddi3, subdi3, lshldi3); negdi2 by floatdidf.  gnulib.c: 32-bit conversions.
MEMBERS2 = fixdfdi floatdidf fixunsdfdi negdi2 adddi3 subdi3 lshldi3 ashrdi3 cmpdi2
MEMBERS1 = fixunsdfsi
OBJS = $(addsuffix .o,$(MEMBERS2)) $(addsuffix .o,$(MEMBERS1)) fixdfsi.o

../lib/libgcc-qd.a: $(OBJS)
	mkdir -p ../lib
	rm -f $@
	$(AR) rc $@ $(OBJS)

gnulib2.c gnulib.c: $(GNUSRC)/gnulib2.c $(GNUSRC)/gnulib.c
	tr -d '\r' < $(GNUSRC)/gnulib2.c > gnulib2.c
	tr -d '\r' < $(GNUSRC)/gnulib.c > gnulib.c

$(addsuffix .o,$(MEMBERS2)): gnulib2.c config.h
	$(GCC) -c -O -DL_$(basename $@) -I. -I$(SDKINC) gnulib2.c -o $@

fixdfsi.o: fixdfsi.s
	$(AS) -o $@ fixdfsi.s

$(addsuffix .o,$(MEMBERS1)): gnulib.c config.h
	$(GCC) -c -O -DL_$(basename $@) -I. -I$(SDKINC) gnulib.c -o $@

clean:
	rm -f *.o gnulib.c gnulib2.c ../lib/libgcc-qd.a

.PHONY: clean
