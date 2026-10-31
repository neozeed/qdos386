/* Adapted from the NS32k standard relocation decoder in binutils 2.5.
   1.94 keeps local relocations as section references, unlike 2.5. */
extern reloc_howto_type ns32k_howto[18], ns32k_invalid_howto;
void
DEFUN(NAME(aout,swap_std_reloc_in), (abfd, bytes, cache_ptr, symbols),
 bfd *abfd AND struct reloc_std_external *bytes AND
 arelent *cache_ptr AND asymbol **symbols)
{
 int r_index, r_extern, r_pcrel, length, storage;
 struct aoutdata *su = (struct aoutdata *)abfd->tdata;
 unsigned int type = bytes->r_type[0];
 cache_ptr->address = bfd_h_get_32(abfd, bytes->r_address);
 r_index = bytes->r_index[0] | bytes->r_index[1]<<8 | bytes->r_index[2]<<16;
 r_extern = (type & 8) != 0;
 r_pcrel = type & 1;
 length = (type >> 1) & 3;
 storage = (type >> 5) & 3;
 if (length > 2 || storage > 2 || (r_extern && r_index >= bfd_get_symcount(abfd))) {
   cache_ptr->howto = &ns32k_invalid_howto;
   cache_ptr->sym_ptr_ptr = NULL; cache_ptr->section = NULL; cache_ptr->addend = 0;
   return;
 }
 cache_ptr->howto = &ns32k_howto[storage*6 + r_pcrel*3 + length];
 MOVE_ADDRESS(0);
}
