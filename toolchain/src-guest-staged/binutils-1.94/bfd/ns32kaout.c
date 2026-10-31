/* NS32k a.out adaptation for BFD 0.18 (binutils 1.94).
   Relocation encodings follow binutils 2.5, Ian Dall's NS32k backend. */
#define ARCH_SIZE 32
#define NS32K_AOUT 1
#define NAME(x,y) CAT3(ns32kaout,_32_,y)
#include "aoutx.h"
#define PAGE_SIZE 4096
#define SEGMENT_SIZE 4096
#define TEXT_START_ADDR 4096
#define DEFAULT_ARCH bfd_arch_ns32k
#define MY(OP) CAT(ns32kaout_,OP)
#define TARGETNAME "a.out-ns32k"
#define MY_get_section_contents bfd_generic_get_section_contents
#define MY_set_arch_mach bfd_default_set_arch_mach
#define MY_close_and_cleanup bfd_generic_close_and_cleanup
#include "aout-target.h"
