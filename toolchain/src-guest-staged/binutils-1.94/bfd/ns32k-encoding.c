/* Encoding routines from binutils 2.5 cpu-ns32k.c; GPLv2 or later.
   Copyright (C) 1990, 1991, 1994 Free Software Foundation, Inc.
   See bfd/COPYING and original cpu-ns32k.c for attribution. */
#include "bfd.h"
#include "sysdep.h"
static long
ns32k_sign_extend(value, bits)
     int value;
     int bits;
{
  value = value & ((1 << bits) - 1);
  return (value & (1 << (bits-1))
	  ? value | (~((1 << bits) - 1))
	  : value);
}

long
ns32k_get_displacement(buffer, offset, size)
     bfd_byte *buffer;
     long offset;
     long size;
{
  long value;
  buffer += offset;
  switch (size)
    {
    case 1:
      value = ns32k_sign_extend (*buffer, 7);
      break;
    case 2:
      value = ns32k_sign_extend(*buffer++, 6);
      value = (value * 256) | (0xff & *buffer);
      break;
    case 4:
      value = ns32k_sign_extend(*buffer++, 6);
      value = (value * 256) | (0xff & *buffer++);
      value = (value * 256) | (0xff & *buffer++);
      value = (value * 256) | (0xff & *buffer);
      break;
    }
  return value;
}

int
ns32k_put_displacement(value, buffer, offset, size)
     long value;
     bfd_byte *buffer;
     long offset;
     long size;
{
  buffer += offset;
  switch (size)
    {
    case 1:
      if (value < -64 || value > 63)
	return -1;
      value&=0x7f;
      *buffer++=value;
      break;
    case 2:
      if (value < -8192 || value > 8191)
	return -1;
      value&=0x3fff;
      value|=0x8000;
      *buffer++=(value>>8);
      *buffer++=value;
      break;
    case 4:
      if (value < -0x20000000 || value >= 0x20000000)
	return -1;
      value|=0xc0000000;
      *buffer++=(value>>24);
      *buffer++=(value>>16);
      *buffer++=(value>>8);
      *buffer++=value;
      break;
    default:
      return -1;
  }
  return 0;
}

long
ns32k_get_immediate(buffer, offset, size)
     bfd_byte *buffer;
     long offset;
     long size;
{
  long value = 0;
  buffer += offset;
  switch (size)
    {
    case 4:
      value = (value * 256) | (*buffer++ & 0xff);
    case 3:
      value = (value * 256) | (*buffer++ & 0xff);
    case 2:
      value = (value * 256) | (*buffer++ & 0xff);
    case 1:
      value = (value * 256) | (*buffer++ & 0xff);
    }
  return value;
}

int
ns32k_put_immediate (value, buffer, offset, size)
     long value;
     bfd_byte *buffer;
     long offset;
     long size;
{
  buffer += offset + size - 1;
  switch (size)
    {
    case 4:
      *buffer-- = (value & 0xff); value >>= 8;
    case 3:
      *buffer-- = (value & 0xff); value >>= 8;
    case 2:
      *buffer-- = (value & 0xff); value >>= 8;
    case 1:
      *buffer-- = (value & 0xff); value >>= 8;
    }
  return 0;
}

/* This is just like the standard perform_relocation except we
 * use get_data and put_data which know about the ns32k
 * storage methods.
 * This is probably a lot more complicated than it needs to be!
 */
