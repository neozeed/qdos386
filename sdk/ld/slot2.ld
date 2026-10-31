OUTPUT_FORMAT("a.out-i386")
OUTPUT_ARCH(i386)
/* QDOS/386 R11H SDK: application profile 2 (origin 0x380000).  The image must
 * fit the profile size of the memory map it runs on (1 MiB on the R9 map, where
 * this profile exists); the argument block is at the top of the profile. */
SECTIONS {
 .text 0x380000 : { *(.text) . = ALIGN(16); }
 .data ALIGN(16) : { *(.data) . = ALIGN(16); }
 .bss ALIGN(16) : { _bss_start = .; *(.bss) *(COMMON) . = ALIGN(16); _bss_end = .; }
}
