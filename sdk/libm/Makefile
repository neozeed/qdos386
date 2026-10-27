# libm.a: the R11C math subset.  Built with the historical GCC 1.40 and GAS
# 1.38 (QDOS386_TOOLS).  Output: ../lib/libm.a.  The archive is linked before
# libc.a (its functions set errno, which lives in libc).
ifndef QDOS386_TOOLS
$(error set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree)
endif
GCC = $(QDOS386_TOOLS)/bin/i386-gcc
AS  = $(QDOS386_TOOLS)/bin/as
AR  = ar
INC = -I../include

OBJS = fabs.o modf.o floor.o ceil.o frexp.o ldexp.o sqrt.o pow.o xmath.o trans.o xtrans.o

../lib/libm.a: $(OBJS)
	mkdir -p ../lib
	rm -f $@
	$(AR) rc $@ $(OBJS)

%.o: %.c ../include/math.h ../include/float.h ../include/errno.h
	$(GCC) -c -O $(INC) $< -o $@

xmath.o: xmath.s
	$(AS) -o $@ xmath.s

xtrans.o: xtrans.s
	$(AS) -o $@ xtrans.s

clean:
	rm -f *.o ../lib/libm.a

.PHONY: clean
