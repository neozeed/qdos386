/* Generate from machine description:

   - some flags HAVE_... saying which simple standard instructions are
   available for this machine.
   Copyright (C) 1987 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include <stdio.h>
#include "config.h"
#include "rtl.h"
#include "obstack.h"

struct obstack obstack;
struct obstack *rtl_obstack = &obstack;

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free
#ifdef LEGACY_HOST64
extern char *xmalloc (), *xrealloc ();
#else
extern int xmalloc ();
#endif
extern void free ();

#ifndef LEGACY_HOST64
void fatal (char * s,int a1,int a2);
#endif
void fancy_abort ();

void
gen_insn (insn)
     rtx insn;
{
  /* Don't mention instructions whose names are the null string.
     They are in the machine description just to be recognized.  */
  if (strlen (XSTR (insn, 0)) == 0)
    return;

  printf ("#define HAVE_%s (%s)\n", XSTR (insn, 0),
	  strlen (XSTR (insn, 2)) ? XSTR (insn, 2) : "1");
  printf ("extern rtx gen_%s ();\n", XSTR (insn, 0));
}

#ifdef LEGACY_HOST64
char *
#else
int
#endif
#ifdef LEGACY_HOST64
xmalloc (size)
     size_t size;
{
#else
xmalloc (size)
{
#endif

  #ifdef LEGACY_HOST64
  register char *val = malloc (size);
#else
  register int val = malloc (size);
#endif

  if (val == 0)
    fatal ("virtual memory exhausted", 0, 0);

  return val;
}

#ifdef LEGACY_HOST64
char *
#else
int
#endif
#ifdef LEGACY_HOST64
xrealloc (ptr, size)
     void *ptr;
     size_t size;
{
#else
xrealloc (ptr, size)
     char *ptr;
     int size;
{
#endif

  #ifdef LEGACY_HOST64
  char *result = realloc (ptr, size);
#else
  int result = realloc (ptr, size);
#endif
  if (!result)
    fatal ("virtual memory exhausted",0, 0);
  return result;
}

#ifdef LEGACY_HOST64
void fatal(const char *s, ...) {
 va_list ap; fprintf(stderr,"genflags: "); va_start(ap,s);
 vfprintf(stderr,s,ap); va_end(ap); fputc('\n',stderr);
 exit(FATAL_EXIT_CODE);
}
#else
void
fatal (s, a1, a2)
     char *s;
{
  fprintf (stderr, "genflags: ");
  fprintf (stderr, s, a1, a2);
  fprintf (stderr, "\n");
  exit (FATAL_EXIT_CODE);
}
#endif


/* More 'friendly' abort that prints the line and file.
   config.h can #define abort fancy_abort if you like that sort of thing.  */

void
fancy_abort ()
{
  fatal ("Internal gcc abort.",0 ,0);
}

int
main (argc, argv)
     int argc;
     char **argv;
{
  rtx desc;
  FILE *infile;
  extern rtx read_rtx ();
  register int c;

  obstack_init (rtl_obstack);

  if (argc <= 1)
    fatal ("No input file name.", 0, 0);

  infile = fopen (argv[1], "r");
  if (infile == 0)
    {
      perror (argv[1]);
      exit (FATAL_EXIT_CODE);
    }

  init_rtl ();

  printf ("/* Generated automatically by the program `genflags'\n\
from the machine description file `md'.  */\n\n");

  /* Read the machine description.  */

  while (1)
    {
      c = read_skip_spaces (infile);
      if (c == EOF)
	break;
      ungetc (c, infile);

      desc = read_rtx (infile);
      if (GET_CODE (desc) == DEFINE_INSN || GET_CODE (desc) == DEFINE_EXPAND)
	gen_insn (desc);
    }

  fflush (stdout);
  exit (ferror (stdout) != 0 ? FATAL_EXIT_CODE : SUCCESS_EXIT_CODE);
}
