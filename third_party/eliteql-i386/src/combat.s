# RC6 combat.s excluding DORND, retained in R1 random.s.
.text
.globl _laser_fire
_laser_fire:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 movb $0,1(%esi)
 xorl %eax,%eax
 cmpb $0,(%esi)
 jne fire_done
 cmpl $0,16(%ebp)
 je fire_done
 cmpb $242,3(%edi)
 jae fire_done
 movl 20(%ebp),%ebx
 testb %bl,%bl
 je fire_done
 movb %bl,%al
 andb $127,%al
 movb %al,1(%esi)
 movb %al,2(%esi)
 pushl 24(%ebp)
 leal 5(%esi),%eax
 pushl %eax
 call _elite_random
 addl $8,%esp
 movw %ax,%dx
 shrw $8,%dx
 andw $7,%ax
 addw %dx,%ax
 addw $92,%ax
 movb %al,4(%esi)
 pushl $0
 leal 5(%esi),%eax
 pushl %eax
 call _elite_random
 addl $8,%esp
 movw %ax,%dx
 shrw $8,%dx
 andw $7,%ax
 addw %dx,%ax
 addw $124,%ax
 movb %al,3(%esi)
 addb $8,3(%edi)
 subb $1,2(%edi)
 jne fire_energy_done
 addb $1,2(%edi)
fire_energy_done:
 testb %bl,%bl
 jns fire_pulse
 xorl %ebx,%ebx
fire_pulse:
 andb $250,%bl
 movb %bl,(%esi)
 movl $1,%eax
fire_done:
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _laser_tick
_laser_tick:
 movl 4(%esp),%edx
 movzbl (%edx),%eax
 movl 8(%esp),%ecx
 andl $1,%ecx
 addw %cx,%ax
 subw $5,%ax
 jns tick_store
 xorl %eax,%eax
tick_store:
 movb %al,(%edx)
 ret
.globl _laser_hit
_laser_hit:
 pushl %ebx
 movl 8(%esp),%edx
 movl 12(%esp),%ecx
 xorl %eax,%eax
 cmpb $0,8(%edx)
 jne hitch_done
 testb %cl,%cl
 js hitch_done
 movb 31(%edx),%cl
 andb $32,%cl
 orb 1(%edx),%cl
 orb 4(%edx),%cl
 jne hitch_done
 movzbl (%edx),%ecx
 imull %ecx,%ecx
 movzbl 3(%edx),%ebx
 imull %ebx,%ebx
 addl %ebx,%ecx
 cmpl $65535,%ecx
 ja hitch_done
 movl 16(%esp),%ebx
 andl $65535,%ebx
 cmpl %ecx,%ebx
 jb hitch_done
 movl $1,%eax
hitch_done:
 popl %ebx
 ret
.globl _ship_angry
_ship_angry:
 movl 4(%esp),%edx
 movl 8(%esp),%eax
 movl 12(%esp),%ecx
 cmpb $7,%al
 je angry_station
 ja angry_done
 cmpb $6,%al
 jne angry_ship
 call angry_station
angry_ship:
 cmpb $0,32(%edx)
 je angry_done
 orb $128,32(%edx)
 movb $2,28(%edx)
 movb $4,30(%edx)
angry_done:
 ret
angry_station:
 testl %ecx,%ecx
 je angry_done
 orb $128,(%ecx)
 ret
.globl _ship_damage
_ship_damage:
 movl 4(%esp),%edx
 movl 8(%esp),%ecx
 movzbl 35(%edx),%eax
 andl $255,%ecx
 subw %cx,%ax
 jns damage_survived
 movl 12(%esp),%ecx
 cmpb $7,%cl
 je damage_station
 orb $128,31(%edx)
 movl $2,%eax
 ret
damage_station:
 movl $3,%eax
 ret
damage_survived:
 movb %al,35(%edx)
 movl $1,%eax
 ret
.globl _player_damage
_player_damage:
 pushl %ebx
 pushl %esi
 movl 12(%esp),%esi
 movl 16(%esp),%ecx
 andl $255,%ecx
 movl 24(%esp),%ebx
 andl $1,%ebx
 decw %bx
 subw %bx,%cx
 movl 20(%esp),%ebx
 movl %esi,%edx
 testb %bl,%bl
 jns damage_front
 incl %edx
damage_front:
 movzbl (%edx),%ebx
 subw %cx,%bx
 js damage_banks
 movb %bl,(%edx)
 xorl %eax,%eax
 jmp player_done
damage_banks:
 movb $0,(%edx)
 andl $255,%ebx
 movzbl 2(%esi),%ecx
 addw %cx,%bx
 movb %bl,2(%esi)
 movl $2,%eax
 testb %bl,%bl
 je player_done
 cmpw $256,%bx
 jb player_done
 movl $1,%eax
player_done:
 popl %esi
 popl %ebx
 ret
.globl _laser_xor
_laser_xor:
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 16(%esp),%esi
 movl 20(%esp),%edi
 movzbl 3(%edi),%eax
 movzbl 4(%edi),%edx
 movl $191,%ecx
 movl $32,%ebx
 call xor_segment
 movl $224,%ebx
 call xor_segment
 movl $48,%ebx
 call xor_segment
 movl $208,%ebx
 call xor_segment
 popl %edi
 popl %esi
 popl %ebx
 ret
.globl _sight_xor
_sight_xor:
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 16(%esp),%esi
 movl $20,%edi
sight_loop:
 movl $128,%eax
 subl %edi,%eax
 movl $96,%edx
 movl $128,%ebx
 addl %edi,%ebx
 movl $96,%ecx
 call xor_segment
 movl $128,%eax
 movl $96,%edx
 subl %edi,%edx
 movl $128,%ebx
 movl $96,%ecx
 addl %edi,%ecx
 call xor_segment
 subl $10,%edi
 jne sight_loop
 popl %edi
 popl %esi
 popl %ebx
 ret
