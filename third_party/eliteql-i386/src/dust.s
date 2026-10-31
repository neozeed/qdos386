# RC6 dust entry and original PIXEL adapter. Generated body uses byte registers.
.text
.globl _dust_run
_dust_run:
 pushl %ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 20(%esp),%esi
 movl 24(%esp),%ecx
 movl 32(%esp),%eax
 movb %al,162(%esi)
 movl 40(%esp),%edi
 movb (%ecx),%al
 movb %al,142(%esi)
 andb $127,%al
 movb %al,146(%esi)
 movb (%ecx),%al
 andb $128,%al
 movb %al,148(%esi)
 xorb $128,%al
 movb %al,149(%esi)
 movb 1(%ecx),%al
 movb %al,144(%esi)
 andb $127,%al
 movb %al,150(%esi)
 movb 1(%ecx),%al
 andb $128,%al
 movb %al,152(%esi)
 xorb $128,%al
 movb %al,153(%esi)
 movzbl 2(%ecx),%eax
 movb %al,154(%esi)
 shlw $6,%ax
 movb %al,156(%esi)
 movb %ah,157(%esi)
 movl 28(%esp),%ecx
 movl (%ecx),%eax
 movl %eax,114(%esi)
 xorl %eax,%eax
 xorl %ebx,%ebx
 xorl %ecx,%ecx
 xorl %edx,%edx
 cmpw $0,36(%esp)
 jne dust_not_init
 movl $56,%ebp
dust_clear_low:
 movb $0,57(%esi,%ebp)
 decl %ebp
 jns dust_clear_low
dust_init_no_clear:
 call dust_original_init
 jmp dust_return
dust_not_init:
 cmpw $4,36(%esp)
 je dust_init_no_clear
 cmpw $1,36(%esp)
 jne dust_not_step
 call dust_original_step
 jmp dust_return
dust_not_step:
 cmpw $2,36(%esp)
 jne dust_redraw
 call dust_original_flip
 jmp dust_return
dust_redraw:
 call dust_original_redraw
dust_return:
 movb %dl,166(%esi)
 movl 28(%esp),%ecx
 movl 114(%esi),%eax
 movl %eax,(%ecx)
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl dust_plot
dust_plot:
 pushal
 testl %edi,%edi
 jz dust_plot_done
 subl $4,%esp
 movb %bl,(%esp)
 movb %al,1(%esp)
 movb 140(%esi),%al
 movb %al,2(%esp)
 movb $0,3(%esp)
 movl %esp,%eax
 pushl $1
 pushl %eax
 pushl %edi
 call _cloud_xor
 addl $16,%esp
dust_plot_done:
 popal
 andb $7,%bl
 cmpb $80,140(%esi)
 setae %dl
 movb %cl,%dh
 movb $0,%al
 ret
