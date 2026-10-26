OUTPUT_FORMAT("a.out-i386")
OUTPUT_ARCH(i386)
/* R5 application slot 1: origin 0x240000, 256 KiB */
SECTIONS {
 .text 0x240000 : { *(.text) . = ALIGN(16); }
 .data ALIGN(16) : { *(.data) . = ALIGN(16); }
 .bss ALIGN(16) : { _bss_start = .; *(.bss) *(COMMON) . = ALIGN(16); _bss_end = .; }
}
