# RC6 tactics.s; original decision order and explicit PRNG entry carries.
.text
.globl _tactics_vector
_tactics_vector:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $8,%esp
 movl 8(%ebp),%esi
 movzwl (%esi),%eax
 movzwl 3(%esi),%edx
 movzwl 6(%esi),%ebx
 movl %eax,%ecx
 orw %dx,%cx
 orw %bx,%cx
 orw $1,%cx
vector_shift:
 testw %cx,%cx
 js vector_scaled
 addw %cx,%cx
 addw %ax,%ax
 addw %dx,%dx
 addw %bx,%bx
 jmp vector_shift
vector_scaled:
 shrw $9,%ax
 orb 2(%esi),%al
 movb %al,-20(%ebp)
 shrw $9,%dx
 orb 5(%esi),%dl
 movb %dl,-18(%ebp)
 shrw $9,%bx
 orb 8(%esi),%bl
 movb %bl,-16(%ebp)
 leal -20(%ebp),%esi
 call normalize_vector
 movl 12(%ebp),%edi
 movb -20(%ebp),%al
 movb %al,(%edi)
 movb -18(%ebp),%al
 movb %al,1(%edi)
 movb -16(%ebp),%al
 movb %al,2(%edi)
 addl $8,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _tactics_dot
_tactics_dot:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $8,%esp
 movl 8(%ebp),%esi
 movswl 16(%ebp),%eax
 addl %eax,%esi
 movl 12(%ebp),%edi
 movl $0,-16(%ebp)
 movl $2,-20(%ebp)
vector_dot_loop:
 movzbl (%edi),%eax
 incl %edi
 movzbl (%esi),%edx
 call tidy_mult
 cmpl $2,-20(%ebp)
 je vector_dot_first
 movw -16(%ebp),%dx
 call sign_add
vector_dot_first:
 movw %ax,-16(%ebp)
 addl $2,%esi
 subl $1,-20(%ebp)
 jns vector_dot_loop
 andl $65535,%eax
 addl $8,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _tactics_due
_tactics_due:
 movl 4(%esp),%edx
 xorl %eax,%eax
 testb $160,31(%edx)
 jne due_done
 cmpb $0,32(%edx)
 jns due_done
 movl 8(%esp),%ecx
 cmpb $8,%cl
 je due_yes
 movl 12(%esp),%ecx
 movl 16(%esp),%eax
 xorw %cx,%ax
 andl $7,%eax
 jne due_no
due_yes:
 movl $1,%eax
 ret
due_no:
 xorl %eax,%eax
due_done:
 ret
.globl _missile_steer
_missile_steer:
 movl $1,%eax
 jmp tactics_entry
.globl _pod_step
_pod_step:
 movl $2,%eax
 jmp tactics_entry
.globl _tactics_step
_tactics_step:
 xorl %eax,%eax
tactics_entry:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $32,%esp
 movl %eax,-44(%ebp)
 movl 8(%ebp),%esi
 movl %esi,-16(%ebp)
 movl 12(%ebp),%edi
 movl %edi,-20(%ebp)
 movl 16(%ebp),%eax
 movl %eax,-24(%ebp)
 movl 20(%ebp),%eax
 movl %eax,-28(%ebp)
 cmpl $2,-44(%ebp)
 je pod_entry
 cmpl $1,-44(%ebp)
 je tactics_vectors
 xorl %eax,%eax
 cmpb $1,(%edi)
 jb tactics_done
 cmpb $6,(%edi)
 ja tactics_done
 je tactics_recharge
 cmpb $2,(%edi)
 je tactics_recharge
 cmpb $0,3(%edi)
 je tactics_recharge
 andb $129,32(%esi)
tactics_recharge:
 movb 1(%edi),%al
 cmpb 35(%esi),%al
 jbe tactics_vectors
 addb $1,35(%esi)
tactics_vectors:
 leal -36(%ebp),%eax
 pushl %eax
 pushl %esi
 call _tactics_vector
 addl $8,%esp
 movl $10,%eax
 call tactics_dot
 movb %al,-33(%ebp)
 movb %al,_tactics_cnt
 cmpb $8,(%edi)
 je tactics_pursue
 xorl %eax,%eax
 call tactics_random
 cmpb $250,%al
 jb tactics_energy
 movl $1,%eax
 call tactics_random
 orb $104,%al
 movb %al,29(%esi)
tactics_energy:
 movzbl 1(%edi),%eax
 shrb $1,%al
 cmpb 35(%esi),%al
 jb tactics_lasers
 shrb $2,%al
 cmpb 35(%esi),%al
 jb tactics_missile
 movl $1,%eax
 call tactics_random
 cmpb $230,%al
 jb tactics_missile
 movb $0,32(%esi)
 movl $2,%eax
 jmp tactics_done
tactics_missile:
 movzbl 31(%esi),%ebx
 andl $7,%ebx
 je tactics_lasers
 xorl %eax,%eax
 call tactics_random
 andb $31,%al
 cmpb %bl,%al
 jae tactics_lasers
 cmpb $0,4(%edi)
 jne tactics_lasers
 subb $1,31(%esi)
 movl $1,%eax
 jmp tactics_done
tactics_lasers:
 movb 1(%esi),%al
 orb 4(%esi),%al
 orb 7(%esi),%al
 andb $224,%al
 jne tactics_manoeuvre
 cmpb $160,-33(%ebp)
 jb tactics_manoeuvre
 orb $64,31(%esi)
 cmpb $163,-33(%ebp)
 jb tactics_manoeuvre
 movzbl 2(%edi),%eax
 movl %eax,%edx
 andl $1,%edx
 pushl %edx
 movzbl 8(%esi),%edx
 pushl %edx
 shrw $1,%ax
 pushl %eax
 pushl -28(%ebp)
 call _player_damage
 addl $16,%esp
 cmpw $2,%ax
 je tactics_death
 subb $1,28(%esi)
 addl $3,%eax
 jmp tactics_done
tactics_death:
 movl $5,%eax
 jmp tactics_done
tactics_manoeuvre:
 movl $1,%eax
 cmpb $3,7(%esi)
 jae tactics_choose
 movb 1(%esi),%al
 orb 4(%esi),%al
 andb $254,%al
 je tactics_steer
 xorl %eax,%eax
tactics_choose:
 call tactics_random
 orb $128,%al
 cmpb 32(%esi),%al
 jae tactics_steer
tactics_pursue:
 xorb $128,-36(%ebp)
 xorb $128,-35(%ebp)
 xorb $128,-34(%ebp)
 xorb $128,-33(%ebp)
tactics_steer:
 movb -33(%ebp),%al
 movb %al,_tactics_cnt
 movl $16,%eax
 call tactics_dot
 xorb $128,%al
 andb $128,%al
 orb $3,%al
 movb %al,30(%esi)
 movb 29(%esi),%al
 andb $127,%al
 cmpb $16,%al
 jae tactics_accel
 movl $22,%eax
 call tactics_dot
 xorb 30(%esi),%al
 andb $128,%al
 xorb $133,%al
 movb %al,29(%esi)
tactics_accel:
 movb -33(%ebp),%al
 testb %al,%al
 js tactics_brake
 cmpb $22,%al
 jb tactics_brake
 movb $3,28(%esi)
 jmp tactics_quiet
tactics_brake:
 andb $127,%al
 cmpb $18,%al
 jb tactics_quiet
 movb $255,28(%esi)
 cmpb $8,(%edi)
 jne tactics_quiet
 movb $254,28(%esi)
tactics_quiet:
 xorl %eax,%eax
tactics_done:
 addl $32,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
pod_entry:
 movl 16(%ebp),%eax
 movb %al,-33(%ebp)
 leal -36(%ebp),%eax
 pushl %eax
 pushl %edi
 call _planet_vector
 addl $8,%esp
 movl $pod_type,%edi
 movl %edi,-20(%ebp)
 jmp tactics_steer
tactics_random:
 pushl %eax
 pushl -24(%ebp)
 call _elite_random
 addl $8,%esp
 ret
tactics_dot:
 pushl %eax
 leal -36(%ebp),%eax
 pushl %eax
 pushl -16(%ebp)
 call _tactics_dot
 addl $12,%esp
 shrw $8,%ax
 ret
.data
.globl _tactics_cnt
_tactics_cnt: .byte 0
pod_type: .byte 11,0
