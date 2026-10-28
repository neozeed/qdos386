/* NS32k relocation adaptation for the 1.94 BFD interface.
   Encoding methods from binutils 2.5 (FSF/Ian Dall); GPLv2 or later. */
#include "bfd.h"
#include "sysdep.h"
long ns32k_get_displacement(bfd_byte *, long, long);
int ns32k_put_displacement(long, bfd_byte *, long, long);
long ns32k_get_immediate(bfd_byte *, long, long);
int ns32k_put_immediate(long, bfd_byte *, long, long);
static bfd_reloc_status_type
ns32k_reloc(bfd *abfd, arelent *rel, asymbol *symbol, PTR data,
            asection *section, bfd *output)
{
  unsigned int type = rel->howto->type;
  int storage = type / 6, size = 1 << rel->howto->size;
  if (type >= 18) return bfd_reloc_outofrange;
  asection *target = symbol ? symbol->section : rel->section;
  long value = symbol && !(symbol->flags & BSF_FORT_COMM) ? symbol->value : 0;
  bfd_byte *loc = (bfd_byte *)data + rel->address;
  long old;
  if (symbol && (symbol->flags & BSF_UNDEFINED) && !output)
    return bfd_reloc_undefined;
  if (rel->address + size > section->size) return bfd_reloc_outofrange;
  /* Keep external symbol references external during ld -r; the final
     link supplies their values.  Only move the PC-relative origin. */
  if (output && symbol) {
    value = rel->howto->pc_relative ? -(long)section->output_offset : 0;
    target = NULL;
  }
  else
  if (target) value += target->output_section->vma + target->output_offset;
  value += rel->addend;
  if (rel->howto->pc_relative && !(output && symbol))
    value -= section->output_section->vma + section->output_offset;
  if (storage == 0) old = ns32k_get_immediate(loc, 0, size);
  else if (storage == 1) old = ns32k_get_displacement(loc, 0, size);
  else if (size == 1) old = *loc;
  else if (size == 2) old = bfd_get_16(abfd, loc);
  else old = (int32_t)bfd_get_32(abfd, loc);
  if (rel->howto->pc_relative && storage != 1) {
    if (size == 1) old = (int8_t)old;
    else if (size == 2) old = (int16_t)old;
    else old = (int32_t)old;
  }
  value += old;
  if (storage != 1) {
    int bits = size * 8;
    long min = -(1L << (bits-1));
    long max = rel->howto->pc_relative ? (1L << (bits-1))-1 : (long)((1ULL << bits)-1);
    /* Interpret 32-bit immediate addends as signed on an LP64 host. */
    if (size == 4 && storage == 0 && old > INT32_MAX) value -= 0x100000000L;
    if (value < min || value > max) return bfd_reloc_overflow;
  }
  if (storage == 0) ns32k_put_immediate(value, loc, 0, size);
  else if (storage == 1) {
    if (ns32k_put_displacement(value, loc, 0, size)) return bfd_reloc_overflow;
  }
  else if (size == 1) *loc = value;
  else if (size == 2) bfd_put_16(abfd, value, loc);
  else bfd_put_32(abfd, value, loc);
  rel->addend = 0;
  /* 1.94 ldwrite.c, rather than BFD, advances relocation addresses. */
  return bfd_reloc_ok;
}
#define H(t,s,p,n) HOWTO(t,0,s,8<<s,p,0,false,true,ns32k_reloc,n,true,0xffffffff,0xffffffff,false)
reloc_howto_type ns32k_howto[18] = {
 H(0,0,false,"IMM8"), H(1,1,false,"IMM16"), H(2,2,false,"IMM32"),
 H(3,0,true,"PCIMM8"), H(4,1,true,"PCIMM16"), H(5,2,true,"PCIMM32"),
 H(6,0,false,"DISP8"), H(7,1,false,"DISP16"), H(8,2,false,"DISP32"),
 H(9,0,true,"PCDISP8"), H(10,1,true,"PCDISP16"), H(11,2,true,"PCDISP32"),
 H(12,0,false,"DATA8"), H(13,1,false,"DATA16"), H(14,2,false,"DATA32"),
 H(15,0,true,"PCDATA8"), H(16,1,true,"PCDATA16"), H(17,2,true,"PCDATA32")
};

reloc_howto_type ns32k_invalid_howto = H(18,0,false,"INVALID");
