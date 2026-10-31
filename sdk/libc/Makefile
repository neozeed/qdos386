# libc.a: the QDOS/386 C89 subset (R9).  Built with the historical GCC 1.40
# through qdos386-gcc's compiler; archived with the host ar (the historical
# tree has none; the archive is read by i386-ld, checked by the link tests).
# Requires QDOS386_TOOLS.  Output: ../lib/libc.a.
ifndef QDOS386_TOOLS
$(error set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree)
endif
GCC = $(QDOS386_TOOLS)/bin/i386-gcc
AR  = ar
AS  = $(QDOS386_TOOLS)/bin/as
INC = -I../include -I../../port/include

OBJS = string.o malloc.o stdlib.o stdio.o ctype.o errno.o strtod.o fpbig.o \
	misc.o time.o scanf.o posixio.o jmp.o envx.o

../lib/libc.a: $(OBJS)
	mkdir -p ../lib
	rm -f $@
	$(AR) rc $@ $(OBJS)

%.o: %.c ../include/qdos386.h ../include/stdio.h ../include/stdlib.h ../include/string.h ../include/stdarg.h ../include/errno.h ../include/float.h fpbig.h
	$(GCC) -c -O $(INC) $< -o $@

# R14A: the C68 libc variant: stdio with the C68 console write hook (_conwrite), console reads that wait in the
# kernel and the C68 end message; posixio with C68 file semantics (full-count reads, deleting an open file).  A separate archive linked BEFORE libc.a by C68 ports (Hack: Makefile.qdos386); libc.a stays unchanged.
../lib/libc68.a: stdio_c68.o posixio_c68.o
	rm -f $@
	$(AR) rc $@ stdio_c68.o posixio_c68.o

stdio_c68.o: stdio.c ../include/qdos386.h ../include/stdio.h fpbig.h
	$(GCC) -c -O $(INC) -DQDOS_C68_CONWRITE stdio.c -o $@

# C68/Unix file semantics: read() returns the whole count unless the file ends; unlink() of an open file (posixio.c, QDOS_C68_POSIX)
posixio_c68.o: posixio.c ../include/qdos386.h
	$(GCC) -c -O $(INC) -DQDOS_C68_POSIX posixio.c -o $@

jmp.o: jmp.s
	$(AS) -o $@ jmp.s

clean:
	rm -f *.o ../lib/libc.a

.PHONY: clean
