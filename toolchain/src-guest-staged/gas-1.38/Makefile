CFLAGS=/Od /G3 /DUSG

OBJ = as.obj xrealloc.obj xmalloc.obj hash.obj hex-value.obj \
atof-generic.obj append.obj messages.obj expr.obj app.obj \
frags.obj input-file.obj input-scrub.obj output-file.obj \
subsegs.obj symbols.obj version.obj \
flonum-const.obj flonum-copy.obj flonum-mult.obj strstr.obj bignum-copy.obj \
obstack.obj error.obj

#gdb.obj gdb-file.obj gdb-symbols.obj gdb-blocks.obj gdb-lines.obj

OBJI860 = i860.obj atof-ieee.obj write-i860.obj read-i860.obj
OBJSPARC = sparc.obj atof-ieee.obj write-sparc.obj read-sparc.obj
OBJNS32K = ns32k.obj atof-ieee.obj write-ns32k.obj read-ns32k.obj
OBJI386 = i386.obj atof-ieee.obj write-i386.obj read-i386.obj
OBJM68K = m68k.obj atof-ieee.obj write-m68k.obj read-m68k.obj
OBJVAX = vax.obj atof-ieee.obj write-vax.obj read-vax.obj atof-vax.obj



default: i860as.exe sparcas.exe ns32kas.exe i386as.exe m68kas.exe vaxas.exe

i860.obj: i860.c
	$(CC) -c -DI860 $(CFLAGS) i860.c

write-i860.obj:	write.c i860.h
	$(CC) -c -DI860 $(CFLAGS) write.c /Fowrite-i860.obj

read-i860.obj: read.c i860.h
	$(CC) -c -DI860 $(CFLAGS) read.c /Foread-i860.obj

i860as.exe: $(OBJ) $(OBJI860)
	link -out:i860as.exe $(OBJ) $(OBJI860)


#sparc
sparc.obj: sparc.c
	$(CC) -c -DSPARC $(CFLAGS) sparc.c

write-sparc.obj:	write.c sparc.h
	$(CC) -c -DSPARC $(CFLAGS) write.c /Fowrite-sparc.obj

read-sparc.obj: read.c sparc.h
	$(CC) -c -DSPARC $(CFLAGS) read.c /Foread-sparc.obj


sparcas.exe: $(OBJ) $(OBJSPARC)
	link -out:sparcas.exe $(OBJ) $(OBJSPARC)


#ns32k
ns32k.obj: ns32k.c
	$(CC) -c -DNS32K $(CFLAGS) ns32k.c

write-ns32k.obj:	write.c 
	$(CC) -c -DNS32K $(CFLAGS) write.c /Fowrite-ns32k.obj

read-ns32k.obj: read.c 
	$(CC) -c -DNS32K $(CFLAGS) read.c /Foread-ns32k.obj


ns32kas.exe: $(OBJ) $(OBJNS32K)
	link -out:ns32kas.exe $(OBJ) $(OBJNS32K)



#i386
i386.obj: i386.c
	$(CC) -c -DI386 $(CFLAGS) i386.c

write-i386.obj:	write.c i386.h
	$(CC) -c -DI386 $(CFLAGS) write.c /Fowrite-i386.obj

read-i386.obj: read.c i386.h
	$(CC) -c -DI386 $(CFLAGS) read.c /Foread-i386.obj


i386as.exe: $(OBJ) $(OBJI386)
	link -out:i386as.exe $(OBJ) $(OBJI386)

#m68k 68020?
m68k.obj: m68k.c
	$(CC) -c -DM68K -DM_SUN $(CFLAGS) m68k.c

write-m68k.obj:	write.c
	$(CC) -c -DM68K $(CFLAGS) write.c /Fowrite-m68k.obj

read-m68k.obj: read.c
	$(CC) -c -DM68K $(CFLAGS) read.c /Foread-m68k.obj


m68kas.exe: $(OBJ) $(OBJM68K)
	link -out:m68kas.exe $(OBJ) $(OBJM68K)



#vax
vax.obj: vax.c
	$(CC) -c -DVAX $(CFLAGS) vax.c

write-vax.obj:	write.c
	$(CC) -c -DVAX $(CFLAGS) write.c /Fowrite-vax.obj

read-vax.obj: read.c
	$(CC) -c -DVAX $(CFLAGS) read.c /Foread-vax.obj

atof-vax.obj: atof-vax.c
	$(CC) -c -DVAX $(CFLAGS) atof-vax.c


vaxas.exe: $(OBJ) $(OBJVAX)
	link -out:vaxas.exe $(OBJ) $(OBJVAX)



clean:
	del /q $(OBJ)
	del /q $(OBJI860)
	del /q $(OBJSPARC)
	del /q $(OBJNS32K)
	del /q $(OBJI386)
	del /q $(OBJM68K)
	del /q $(OBJVAX)
	del /q i860as.exe sparcas.exe ns32kas.exe i386as.exe m68kas.exe vaxas.exe