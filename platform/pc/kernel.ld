OUTPUT_FORMAT("a.out-i386")
OUTPUT_ARCH(i386)
SECTIONS {
 .text 0x100000 : { *(.text) . = ALIGN(16); }
 .data ALIGN(16) : { *(.data) }
 .bss SIZEOF(.data) + ADDR(.data) : { _bss_start = .; *(.bss) [COMMON] . = ALIGN(4); _bss_end = .; }
}
