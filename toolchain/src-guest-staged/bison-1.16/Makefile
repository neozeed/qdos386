# Generated automatically from Makefile.in by configure.
# Makefile for bison
# Copyright (C) 1984, 1989, 1991 Bob Corbett and Free Software Foundation, Inc.
# 
# This file is part of Bison, the GNU Compiler Compiler.
# 
# Bison is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# Bison is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Bison; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

SHELL = /bin/sh

#### Start of system configuration section. ####

srcdir = .


CC = cl -Od -G3
INSTALL = /usr/bin/install -c
INSTALLDATA = /usr/bin/install -c -m 644

# Things you might add to DEFS:
# -DSTDC_HEADERS	If you have ANSI C headers and libraries.
# -DUSG			If you have System V/ANSI C string and
#			memory functions and headers.
DEFS = /DUSG /D__STDC__

CDEBUG = #/Od /Zi
CFLAGS = $(CDEBUG) $(DEFS)
LDFLAGS = #-g

LIBS = 

# Some System V machines do not come with libPW.  If this is true, use
# the GNU alloca.o here.
ALLOCA = 

prefix = /usr/local

# where the installed binary goes
bindir = $(prefix)/bin

# where the parsers go
datadir = .
#need to put the data files in the current working directory
#otherwise it'll have a staticly compiled msys path :(
#$(prefix)/lib

# where the info files go
infodir = $(prefix)/info

# where manual pages go and what their extensions should be
mandir = $(prefix)/man/man$(manext)
manext = 1

#### End of system configuration section. ####

# names of parser files
PFILE = bison.simple
PFILE1 = bison.hairy

PFILES = -DXPFILE=\"$(datadir)/$(PFILE)\" \
	 -DXPFILE1=\"$(datadir)/$(PFILE1)\"

OBJECTS = LR0.obj allocate.obj closure.obj conflicts.obj derives.obj files.obj	\
	  getargs.obj gram.obj lalr.obj lex.obj					\
	  main.obj nullable.obj output.obj print.obj reader.obj reduce.obj symtab.obj	\
	  warshall.obj version.obj	bcopy.obj					\
	  getopt.obj getopt1.obj $(ALLOCA)

all: bison

clean:
	del /Q *.obj core bison

distclean: realclean
	del /Q TAGS Makefile config.status

realclean: distclean
	del /Q bison.info*



bison: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)


bison.info: bison.texinfo
	makeinfo bison.texinfo

TAGS: *.c *.h
	etags *.c *.h

# This file is different to pass the parser file names to the compiler.
files.obj: files.c
	$(CC) -c $(CFLAGS) $(PFILES) files.c $(OUTPUT_OPTION)

LR0.obj: system.h machine.h new.h gram.h state.h
closure.obj: system.h machine.h new.h gram.h
conflicts.obj: system.h machine.h new.h files.h gram.h state.h
derives.obj: system.h new.h types.h gram.h
files.obj: system.h files.h new.h gram.h
getargs.obj: system.h files.h
lalr.obj: system.h machine.h types.h state.h new.h gram.h
lex.obj: system.h files.h symtab.h lex.h
main.obj: system.h machine.h
nullable.obj: system.h types.h gram.h new.h
output.obj: system.h machine.h new.h files.h gram.h state.h
print.obj: system.h machine.h new.h files.h gram.h state.h
reader.obj: system.h files.h new.h symtab.h lex.h gram.h
reduce.obj: system.h machine.h files.h new.h gram.h
symtab.obj: system.h new.h symtab.h gram.h
warshall.obj: system.h machine.h
