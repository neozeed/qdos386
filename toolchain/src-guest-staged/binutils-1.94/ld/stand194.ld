/* Binutils 1.94 script grammar: zero-based NS32k emulator image. */
OUTPUT_FORMAT("a.out-ns32k")
OUTPUT_ARCH(ns32k)
ENTRY(_start)
SECTIONS
{
 .text 0 : { *(.text) }
 .data ALIGN(16) : { *(.data) }
 .bss SIZEOF(.data) + ADDR(.data) :
 { *(.bss) [COMMON] . = ALIGN(4); }
}
