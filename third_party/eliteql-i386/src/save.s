# RC6 commander I/O sequence. File bytes remain unchanged (EQA2, 144/148 bytes).
# Platform calls return the original error value; lengths/handles are native.
.text
.globl _commander_io
_commander_io:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $8,%esp
 movl 8(%ebp),%esi
 movl $1,%eax
 cmpw $0,12(%ebp)
 je save_open
 movl $3,%eax
save_open:
 leal -16(%ebp),%edx
 pushl %edx
 pushl %eax
 pushl $_commander_filename
 call _platform_file_open
 addl $12,%esp
 testl %eax,%eax
 jnz save_return
 movl $144,%edi
 cmpw $0,12(%ebp)
 je save_transfer
 movl $148,%edi
save_transfer:
 leal -20(%ebp),%eax
 pushl %eax
 movzwl 12(%ebp),%eax
 pushl %eax
 pushl %edi
 pushl %esi
 pushl -16(%ebp)
 call _platform_file_transfer
 addl $20,%esp
 movl %eax,%ebx
 testl %ebx,%ebx
 jnz save_close
 cmpw -20(%ebp),%di
 jne save_short
 cmpw $0,12(%ebp)
 jne save_close
 cmpl $0x32415145,(%esi)
 jne save_close
 leal -20(%ebp),%eax
 pushl %eax
 pushl $0
 pushl $4
 leal 144(%esi),%eax
 pushl %eax
 pushl -16(%ebp)
 call _platform_file_transfer
 addl $20,%esp
 movl %eax,%ebx
 testl %ebx,%ebx
 jnz save_close
 cmpw $4,-20(%ebp)
 je save_close
save_short:
 movl $-10,%ebx
save_close:
 pushl -16(%ebp)
 call _platform_file_close
 addl $4,%esp
 testl %ebx,%ebx
 jz save_return
 movl %ebx,%eax
save_return:
 addl $8,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.data
.align 2
.globl _commander_filename
_commander_filename: .byte 0,14
 .ascii "WIN1_ELITESAVE"
 .space 4
