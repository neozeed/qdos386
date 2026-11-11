OUTPUT_FORMAT("a.out-i386")
OUTPUT_ARCH(i386)
SECTIONS {
 .text 0 : { *(.text) . = ALIGN(16); }
 .data ALIGN(16) : { *(.data) }
 .bss SIZEOF(.data) + ADDR(.data) : { *(.bss) [COMMON] }
}
