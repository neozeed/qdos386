# libc.a: the QDOS/386 C89 subset (R9).  Built with the historical GCC 1.40
# through qdos386-gcc's compiler; archived with the host ar (the historical
# tree has none; the archive is read by i386-ld, checked by the link tests).
# Requires QDOS386_TOOLS.  Output: ../lib/libc.a.
ifndef QDOS386_TOOLS
$(error set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree)
endif
GCC = $(QDOS386_TOOLS)/bin/i386-gcc
AR  = ar
INC = -I../include -I../include

OBJS = string.o malloc.o stdlib.o stdio.o

../lib/libc.a: $(OBJS)
	mkdir -p ../lib
	rm -f $@
	$(AR) rc $@ $(OBJS)

%.o: %.c ../include/qdos386.h ../include/stdio.h ../include/stdlib.h ../include/string.h ../include/stdarg.h
	$(GCC) -c -O $(INC) $< -o $@

clean:
	rm -f *.o ../lib/libc.a

.PHONY: clean
