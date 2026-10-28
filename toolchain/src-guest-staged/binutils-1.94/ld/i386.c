/* NOTE: If there are angle brackets here: VAX then this is a
 * template file (ldtemplate), intended for processing by sed.
 * Otherwise, this file has already been processed by sed,
 * and customized for a particular emulation target.
 * In that DO NOT EDIT the file; edit ldtemplate instead.
 */

/* emulate the original gld for the given VAX
   Copyright (C) 1991 Free Software Foundation, Inc.
   Written by Steve Chamberlain steve@cygnus.com

This file is part of GLD, the Gnu Linker.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 *  $Id:#
 */

#define TARGET_IS_VAX


#include "bfd.h"
#include "sysdep.h"
#include "ld.h"
#include "config.h"
#include "ldemul.h"
#include "ldfile.h"
#include "ldmisc.h"

extern  boolean lang_float_flag;


extern enum bfd_architecture ldfile_output_architecture;
extern unsigned long ldfile_output_machine;
extern char *ldfile_output_machine_name;

extern bfd *output_bfd;



static void gldi386_before_parse()
{
#ifdef TARGET_IS_M88KBCS
  extern char lprefix;
  lprefix = '@';
#else
#ifndef TARGET_ /* I.e., if not generic */
  ldfile_output_architecture = bfd_arch_i386;
#endif
#endif
}


static void 
gldi386_after_parse()
{

}

static void
gldi386_after_allocation()
{

}

static void
gldi386_before_allocation()
{

}


static void
gldi386_set_output_arch()
{
  /* Set the output architecture and machine if possible */
  bfd_set_arch_mach(output_bfd,
	            ldfile_output_architecture, ldfile_output_machine);
}

static char *
gldi386_choose_target()
{
  char *from_outside = getenv(TARGET_ENVIRON);
  if (from_outside != (char *)NULL)
    return from_outside;
  return "a.out-i386";
}

static void
gldi386_syslib()
{
  info("%S SYSLIB ignored\n");
}

static void
gldi386_hll(ignore)
char  *ignore;
{
  info("%S HLL ignored\n");
}

static char *gldi386_script =  
#include "i386.x"
;
static char *gldi386_script_option_Ur  =  
#include "i386.xu"
;
static char *gldi386_script_option_r  =  
#include "i386.xr"
;
static char *gldi386_script_option_n  =  /* Used with -n flag. */
#include "i386.xn"
;
static char *gldi386_script_option_N  =  /* Used with -N flag. */
#include "i386.xN"
;

static char *gldi386_get_script()
{			     
  extern ld_config_type config;
  if (config.relocateable_output == true &&
      config.build_constructors == true) {
    return gldi386_script_option_Ur;
  }
  if (config.relocateable_output == true) {
    return gldi386_script_option_r;
  }
  if (!config.text_read_only)
    return gldi386_script_option_N;
  if (!config.magic_demand_paged)
    return gldi386_script_option_n;
  return gldi386_script;
}
struct ld_emulation_xfer_struct ld_gldi386_emulation = 
{
  gldi386_before_parse,
  gldi386_syslib,
  gldi386_hll,
  gldi386_after_parse,
  gldi386_after_allocation,
  gldi386_set_output_arch,
  gldi386_choose_target,
  gldi386_before_allocation,
  gldi386_get_script,
};
