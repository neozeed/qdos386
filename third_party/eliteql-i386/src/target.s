# RC6 target.s: 23-bit relative vectors, target missiles, player initialization.
.text
.globl _relative_vector
_relative_vector:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $16,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 movl 16(%ebp),%ebx
 movl $2,-16(%ebp)
relative_axis:
 movzbl 2(%esi),%eax
 shll $8,%eax
 movb 1(%esi),%al
 shll $8,%eax
 movb (%esi),%al
 movzbl 2(%edi),%edx
 shll $8,%edx
 movb 1(%edi),%dl
 shll $8,%edx
 movb (%edi),%dl
 movl %eax,%ecx
 andl $0x800000,%ecx
 movl %ecx,-20(%ebp)
 movl %edx,%ecx
 andl $0x800000,%ecx
 xorl $0x800000,%ecx
 movl %ecx,-24(%ebp)
 andl $0x7fffff,%eax
 andl $0x7fffff,%edx
 cmpl -20(%ebp),%ecx
 jne relative_sub
 movl $0,-28(%ebp)
 cmpl $0,-20(%ebp)
 je relative_add
 movl $1,-28(%ebp)
relative_add:
 addl %edx,%eax
 andl $0x7fffff,%eax
 jmp relative_store
relative_sub:
 movl $1,-28(%ebp)
 subl %edx,%eax
 jnc relative_store
 movl $0,-28(%ebp)
 negl %eax
 movl -24(%ebp),%ecx
 movl %ecx,-20(%ebp)
relative_store:
 orl -20(%ebp),%eax
 movb %al,(%ebx)
 shrl $8,%eax
 movb %al,1(%ebx)
 shrl $8,%eax
 movb %al,2(%ebx)
 addl $3,%ebx
 addl $3,%esi
 addl $3,%edi
 subl $1,-16(%ebp)
 jns relative_axis
 movl -28(%ebp),%eax
 addl $16,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _friendly_missile
_friendly_missile:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $48,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 testl %edi,%edi
 je friendly_destroy
 movl $35,%ecx
friendly_copy:
 movb (%esi,%ecx),%al
 movb %al,-60(%ebp,%ecx)
 decl %ecx
 jns friendly_copy
 leal -60(%ebp),%eax
 pushl %eax
 pushl %edi
 pushl %esi
 call _relative_vector
 addl $12,%esp
 movl %eax,%ebx
 movb -58(%ebp),%al
 orb -55(%ebp),%al
 orb -52(%ebp),%al
 andb $127,%al
 orb -59(%ebp),%al
 orb -56(%ebp),%al
 orb -53(%ebp),%al
 jne friendly_far
 cmpw $7,16(%ebp)
 je friendly_destroy
 testb $32,31(%edi)
 jne friendly_destroy
 orb $128,31(%edi)
 orb $128,31(%esi)
 movl $8,%eax
 jmp friendly_done
friendly_destroy:
 orb $128,31(%esi)
 movl $6,%eax
 jmp friendly_done
friendly_far:
 pushl %ebx
 pushl 20(%ebp)
 call _elite_random
 addl $8,%esp
 cmpb $16,%al
 jae friendly_steer
 testb $1,32(%edi)
 je friendly_steer
 movl $7,%eax
 jmp friendly_done
friendly_steer:
 movb $8,-24(%ebp)
 pushl $0
 pushl 20(%ebp)
 leal -24(%ebp),%eax
 pushl %eax
 leal -60(%ebp),%eax
 pushl %eax
 call _missile_steer
 addl $16,%esp
 movb -32(%ebp),%al
 movb %al,28(%esi)
 movb -31(%ebp),%al
 movb %al,29(%esi)
 movb -30(%ebp),%al
 movb %al,30(%esi)
 xorl %eax,%eax
friendly_done:
 addl $48,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _player_missile_init
_player_missile_init:
 movl 4(%esp),%edx
 movl $35,%ecx
player_zero:
 movb $0,(%edx,%ecx)
 decl %ecx
 jns player_zero
 movb $28,3(%edx)
 movb $128,5(%edx)
 movb $14,6(%edx)
 movb $96,14(%edx)
 movb $96,18(%edx)
 movb $224,22(%edx)
 movl 8(%esp),%eax
 addb %al,%al
 orb $128,%al
 movb %al,32(%edx)
 movl 12(%esp),%eax
 addb %al,%al
 movb %al,27(%edx)
 movb $2,35(%edx)
 ret
