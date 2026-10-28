/* NOTE: If there are angle brackets here: NS32K then this is a
 * template file (ldtemplate), intended for processing by sed.
 * Otherwise, this file has already been processed by sed,
 * and customized for a particular emulation target.
 * In that DO NOT EDIT the file; edit ldtemplate instead.
 */

/* emulate the original gld for the given NS32K
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

#define TARGET_IS_NS32K


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



static void gldns32k_before_parse()
{
#ifdef TARGET_IS_M88KBCS
  extern char lprefix;
  lprefix = '@';
#else
#ifndef TARGET_ /* I.e., if not generic */
  ldfile_output_architecture = bfd_arch_ns32k;
#endif
#endif
}


static void 
gldns32k_after_parse()
{

}

static void
gldns32k_after_allocation()
{

}

static void
gldns32k_before_allocation()
{

}


static void
gldns32k_set_output_arch()
{
  /* Set the output architecture and machine if possible */
  bfd_set_arch_mach(output_bfd,
	            ldfile_output_architecture, ldfile_output_machine);
}

static char *
gldns32k_choose_target()
{
  char *from_outside = getenv(TARGET_ENVIRON);
  if (from_outside != (char *)NULL)
    return from_outside;
  return "a.out-ns32k";
}

static void
gldns32k_syslib()
{
  info("%S SYSLIB ignored\n");
}

static void
gldns32k_hll(ignore)
char  *ignore;
{
  info("%S HLL ignored\n");
}

static char *gldns32k_script =  
#include "ns32k.x"
;
static char *gldns32k_script_option_Ur  =  
#include "ns32k.xu"
;
static char *gldns32k_script_option_r  =  
#include "ns32k.xr"
;
static char *gldns32k_script_option_n  =  /* Used with -n flag. */
#include "ns32k.xn"
;
static char *gldns32k_script_option_N  =  /* Used with -N flag. */
#include "ns32k.xN"
;

static char *gldns32k_get_script()
{			     
  extern ld_config_type config;
  if (config.relocateable_output == true &&
      config.build_constructors == true) {
    return gldns32k_script_option_Ur;
  }
  if (config.relocateable_output == true) {
    return gldns32k_script_option_r;
  }
  if (!config.text_read_only)
    return gldns32k_script_option_N;
  if (!config.magic_demand_paged)
    return gldns32k_script_option_n;
  return gldns32k_script;
}
struct ld_emulation_xfer_struct ld_gldns32k_emulation = 
{
  gldns32k_before_parse,
  gldns32k_syslib,
  gldns32k_hll,
  gldns32k_after_parse,
  gldns32k_after_allocation,
  gldns32k_set_output_arch,
  gldns32k_choose_target,
  gldns32k_before_allocation,
  gldns32k_get_script,
};
