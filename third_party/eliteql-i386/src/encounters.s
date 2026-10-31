# RC6 encounters.s. Explicit carry/overflow propagation and pool insertion.
# Local -16 carry, -20 overflow, -24 type, -28 BAD, -32 pirate counter;
# -36 out, -40 RNG, -44 params, -48 cargo, -84..-49 INWK scratch.
.text
.globl _encounter_step
_encounter_step:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $72,%esp
 movl 8(%ebp),%eax
 movl %eax,-36(%ebp)
 movw $0,(%eax)
 movl 12(%ebp),%ebx
 movl %ebx,-40(%ebp)
 movl 16(%ebp),%esi
 movl %esi,-44(%ebp)
 movl 20(%ebp),%eax
 movl %eax,-48(%ebp)
 leal -84(%ebp),%edi
 movzbl 6(%esi),%eax
 andl $1,%eax
 movl %eax,-16(%ebp)
 call en_random
 cmpb $35,%al
 jae en_police_carry
 cmpb $3,1(%esi)
 jae en_police_carry
 call en_zero
 movb $38,7(%edi)
 movl $0,-16(%ebp)
 call en_random
 movb %al,(%edi)
 movb %dl,3(%edi)
 andb $128,%al
 movb %al,2(%edi)
 andb $128,%dl
 movb %dl,5(%edi)
 shll $1,-16(%ebp)
 movl -16(%ebp),%eax
 movb %al,1(%edi)
 movl $0,-16(%ebp)
 call en_random
 cmpl $0,-20(%ebp)
 jne en_trader
 movb %al,%cl
 orb $111,%cl
 movb %cl,29(%edi)
 cmpb $0,(%esi)
 jne en_police
 cmpl $0,-16(%ebp)
 jne en_pitch
 movb %dl,%al
 andb $31,%al
 orb $16,%al
 movb %al,27(%edi)
 jmp en_rock
en_pitch:
 orb $127,%dl
 movb %dl,30(%edi)
en_rock:
 call en_random
 cmpb $5,%al
 jb en_canister
 movl $9,%eax
 jmp en_benign_add
en_canister:
 movl $10,%eax
en_benign_add:
 call en_add
 jmp en_police
en_trader:
 movb %al,%cl
 andb $1,%cl
 movb %cl,31(%edi)
 shrb $1,%al
 movb %al,32(%edi)
 movb %al,29(%edi)
 andb $31,%al
 orb $16,%al
 movb %al,27(%edi)
 movl $6,%eax
 call en_add
 jmp en_done
en_police_carry:
 movl $1,-16(%ebp)
en_police:
 cmpb $0,(%esi)
 jne en_done
 movl -48(%ebp),%ecx
 movzbl 3(%ecx),%eax
 addb 6(%ecx),%al
 movl %eax,%edx
 shrw $7,%dx
 andl $1,%edx
 addb %al,%al
 addb 10(%ecx),%al
 addb %dl,%al
 movl $0,-16(%ebp)
 testb $128,%al
 je en_bad_shift
 movl $1,-16(%ebp)
en_bad_shift:
 addb %al,%al
 cmpb $0,2(%esi)
 je en_bad_ready
 orb 4(%esi),%al
en_bad_ready:
 movb %al,-28(%ebp)
 call en_ze
 movl $1,-16(%ebp)
 cmpb -28(%ebp),%al
 jae en_no_cop
 movl $2,%eax
 call en_add
 cmpl $0,-16(%ebp)
 je en_no_cop
 addb $1,2(%esi)
en_no_cop:
 cmpb $0,2(%esi)
 jne en_done
 subb $1,5(%esi)
 jns en_done
 addb $1,5(%esi)
 call en_random
 cmpb $0,3(%esi)
 je en_hostile
 cmpb $90,%al
 jae en_done_set_carry
 andb $7,%al
 cmpb 3(%esi),%al
 jb en_done_clear_carry
 movl $1,-16(%ebp)
en_hostile:
 call en_ze
 cmpb $200,%al
 jae en_pirates
 addb $1,5(%esi)
 andb $3,%al
 addb $3,%al
 movw %ax,-24(%ebp)
 xorl %ecx,%ecx
 cmpb $200,%dl
 jb en_hunter_ecm
 movl $1,%ecx
en_hunter_ecm:
 addb %dl,%dl
 orb %cl,%dl
 orb $192,%dl
 movb %dl,32(%edi)
 movzwl -24(%ebp),%eax
 call en_add
 jmp en_done
en_pirates:
 andl $3,%eax
 movb %al,5(%esi)
 movl %eax,-32(%ebp)
 movl $1,-16(%ebp)
en_pirate_loop:
 call en_random
 andb $3,%al
 orb $1,%al
 call en_add
 subl $1,-32(%ebp)
 jns en_pirate_loop
en_done:
 movl -16(%ebp),%eax
 addl $72,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
en_done_set_carry:
 movl $1,-16(%ebp)
 jmp en_done
en_done_clear_carry:
 movl $0,-16(%ebp)
 jmp en_done
en_zero:
 movl $35,%ecx
en_clear:
 movb $0,(%edi,%ecx)
 decl %ecx
 jns en_clear
 movb $96,18(%edi)
 movb $96,22(%edi)
 movb $224,14(%edi)
 ret
en_ze:
 call en_zero
 call en_random
 andb $128,%al
 movb %al,2(%edi)
 movb %dl,%al
 andb $128,%al
 movb %al,5(%edi)
 movb $32,1(%edi)
 movb $32,4(%edi)
 movb $32,7(%edi)
 xorl %ecx,%ecx
 cmpb $245,%dl
 jb en_ze_ecm
 movl $1,%ecx
en_ze_ecm:
 addb %dl,%dl
 orb %cl,%dl
 orb $192,%dl
 movb %dl,32(%edi)
 movl $0,-16(%ebp)
 jmp en_random
en_random:
 movzbl (%ebx),%eax
 movl %eax,%ecx
 shrw $7,%cx
 addw %ax,%ax
 orw -16(%ebp),%ax
 andl $255,%eax
 movzbl 2(%ebx),%edx
 movb %al,2(%ebx)
 addw %dx,%ax
 addw %cx,%ax
 movb %al,(%ebx)
 shrw $8,%ax
 movzbl 1(%ebx),%edx
 movzbl 3(%ebx),%ecx
 addw %dx,%ax
 addw %cx,%ax
 movb %al,1(%ebx)
 movb %dl,3(%ebx)
 pushl %eax
 shrw $8,%ax
 movzwl %ax,%eax
 movl %eax,-16(%ebp)
 movl %edx,%eax
 xorl %edx,%ecx
 notw %cx
 xorw (%esp),%ax
 andw %cx,%ax
 andl $128,%eax
 movl %eax,-20(%ebp)
 popl %eax
 andl $255,%eax
 ret
en_add:
 cmpl $0,_encounter_pool
 je en_add_legacy
 pushl %ecx
 pushl %edx
 pushl %eax
 pushl %eax
 call _universe_cost
 addl $4,%esp
 pushl %eax
 pushl 4(%esp)
 pushl _encounter_pool
 call _universe_add
 addl $12,%esp
 movl $0,-16(%ebp)
 testl %eax,%eax
 js en_pool_denied
 movl $1,-16(%ebp)
en_pool_denied:
 popl %eax
 popl %edx
 popl %ecx
 cmpl $0,-16(%ebp)
 je en_add_done
 jmp en_add_record
en_add_legacy:
 movl $0,-16(%ebp)
 movl -36(%ebp),%ecx
 pushl %eax
 movzwl (%ecx),%eax
 xchgb %al,%ah
 movzbl 7(%esi),%ecx
 cmpw %ax,%cx
 jbe en_add_denied
 cmpw $5,%ax
 jae en_add_denied
 popl %eax
 jmp en_add_record
en_add_denied:
 popl %eax
 ret
en_add_record:
 pushl %eax
 pushl %edx
 pushl %esi
 movl -36(%ebp),%esi
 movzwl (%esi),%ecx
 xchgb %cl,%ch
 imull $40,%ecx,%ecx
 leal 2(%esi,%ecx),%esi
 xchgb %al,%ah
 movw %ax,(%esi)
 addl $2,%esi
 xorl %ecx,%ecx
en_add_copy:
 movb (%edi,%ecx),%al
 movb %al,(%esi,%ecx)
 incl %ecx
 cmpl $36,%ecx
 jb en_add_copy
 movw $0,36(%esi)
 movl -36(%ebp),%esi
 movzwl (%esi),%eax
 xchgb %al,%ah
 incw %ax
 xchgb %al,%ah
 movw %ax,(%esi)
 movl $1,-16(%ebp)
 popl %esi
 popl %edx
 popl %eax
en_add_done:
 ret
.data
.globl _encounter_pool
_encounter_pool: .long 0
