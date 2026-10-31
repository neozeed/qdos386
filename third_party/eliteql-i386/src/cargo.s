# RC6 cargo.s. Preserve original byte-ADC carry across all 13 tonne stocks.
.text
.globl _cargo_room
_cargo_room:
 pushl %ebx
 pushl %esi
 movl 12(%esp),%esi
 movl 16(%esp),%eax
 cmpw $12,%ax
 ja cargo_kg
 movl 20(%esp),%eax
 andl $255,%eax
 movl $12,%ecx
 movl $1,%edx
cargo_sum:
 movzbl (%esi,%ecx),%ebx
 addw %bx,%ax
 addw %dx,%ax
 movw %ax,%dx
 shrw $8,%dx
 andw $255,%ax
 decl %ecx
 jns cargo_sum
 cmpb 24(%esp),%al
 setb %al
 andl $1,%eax
 jmp cargo_return
cargo_kg:
 movswl %ax,%eax
 movb (%esi,%eax),%cl
 movl 20(%esp),%eax
 addb %cl,%al
 cmpb $200,%al
 setb %al
 andl $1,%eax
cargo_return:
 popl %esi
 popl %ebx
 ret
.globl _scoop_step
_scoop_step:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 8(%ebp),%esi
 movl 12(%ebp),%ebx
 movb 31(%esi),%al
 andb $160,%al
 orb 1(%esi),%al
 orb 4(%esi),%al
 orb 7(%esi),%al
 jne scoop_none
 movb (%esi),%al
 orb 3(%esi),%al
 orb 6(%esi),%al
 andb $192,%al
 jne scoop_none
 testb %bl,%bl
 js scoop_none
 cmpb $7,%bl
 je scoop_none
 cmpb $8,%bl
 je scoop_none
 cmpb $10,%bl
 jb scoop_collision
 movl 16(%ebp),%eax
 andb 5(%esi),%al
 jns scoop_collision
 movl $3,%edi
 cmpb $11,%bl
 je scoop_room
 pushl $0
 pushl 24(%ebp)
 call _elite_random
 addl $8,%esp
 movl %eax,%edi
 andl $7,%edi
scoop_room:
 movl 20(%ebp),%ebx
 pushl 28(%ebp)
 pushl $1
 pushl %edi
 pushl %ebx
 call _cargo_room
 addl $16,%esp
 orb $128,31(%esi)
 testl %eax,%eax
 je scoop_full
 addb $1,(%ebx,%edi)
 movl $1,%eax
 jmp scoop_done
scoop_full:
 movl $2,%eax
 jmp scoop_done
scoop_collision:
 orb $128,31(%esi)
 movl $3,%eax
 jmp scoop_done
scoop_none:
 xorl %eax,%eax
scoop_done:
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _debris_count
_debris_count:
 movl 4(%esp),%eax
 cmpb $7,%al
 jae debris_none
 pushl $0
 pushl 16(%esp)
 call _elite_random
 addl $8,%esp
 testb %al,%al
 jns debris_none
 andl 8(%esp),%eax
 andl $255,%eax
 ret
debris_none:
 xorl %eax,%eax
 ret
.globl _cargo_tumble
_cargo_tumble:
 pushl %esi
 movl 8(%esp),%esi
 pushl $1
 pushl 16(%esp)
 call _elite_random
 addl $8,%esp
 movb $0,32(%esi)
 movb %al,%dl
 addb %dl,%dl
 movb %dl,30(%esi)
 movb $127,29(%esi)
 testb %al,%al
 jns tumble_speed
 movb $255,29(%esi)
tumble_speed:
 shrl $16,%eax
 andb $15,%al
 movb %al,27(%esi)
 popl %esi
 ret
.data
.globl _ship_fit
_ship_fit: .byte 22,15,0,0,0,0,0,0
