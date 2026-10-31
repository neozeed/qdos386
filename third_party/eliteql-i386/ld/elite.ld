OUTPUT_FORMAT("a.out-i386")
OUTPUT_ARCH(i386)
/* QDOS/386 SDK: application slot 1, origin 0x240000 (1 MiB on the R9 map, 256 KiB on the default map; argument block at the top) */
SECTIONS {
 .text 0x240000 : { *(.text) . = ALIGN(16); }
 .data ALIGN(16) : { *(.data) . = ALIGN(16); }
 .bss ALIGN(16) : { _bss_start = .; *(.bss) *(COMMON) . = ALIGN(16); _bss_end = .; }
}
