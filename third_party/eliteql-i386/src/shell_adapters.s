# Private shell register interfaces to the existing native cdecl helpers.
# Text pointer ESI, line EBX, column EBP; decimal ECX/EAX/EDX.
.text
.globl asm_text
asm_text:
 pushal
 pushl %ebx
 pushl %esi
 pushl _elite_screen
 call _asm_text
 addl $12,%esp
 popal
 ret
.globl asm_text_at
asm_text_at:
 pushal
 pushl %ebp
 pushl %ebx
 pushl %esi
 pushl _elite_screen
 call _asm_text_at
 addl $16,%esp
 popal
 ret
.globl asm_decimal
asm_decimal:
 pushal
 pushl %edx
 pushl %eax
 pushl %ecx
 call _asm_decimal
 addl $12,%esp
 popal
 ret
.globl save_copy
save_copy:
 pushal
 pushl %ebx
 pushl $save_parts
 pushl $save_buffer
 call _save_copy
 addl $12,%esp
 popal
 ret
.globl save_checksum
save_checksum:
 pushl $save_buffer
 call _save_checksum
 addl $4,%esp
 ret
