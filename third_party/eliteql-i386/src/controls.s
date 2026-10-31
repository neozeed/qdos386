# RC6 controls.s. d0=ax d1=dx d2=bx d3=cl d4=ebp a0=esi a1=edi.
# Byte/word operations intentionally retain the original wrap and comparisons.
.text
.globl _control_step
_control_step:
 pushl %ebx
 pushl %esi
 pushl %edi
 pushl %ebp
 movl 20(%esp),%esi
 movl 24(%esp),%edi
 xorl %eax,%eax
 movb (%esi),%al
 movl $7,%edx
 cmpb $0,(%edi)
 je control_right
 call control_bump2
control_right:
 cmpb $0,1(%edi)
 je control_pitch
 call control_redu2
control_pitch:
 movb %al,(%esi)
 movb 1(%esi),%al
 movl $14,%edx
 cmpb $0,2(%edi)
 je control_down
 call control_redu2
control_down:
 cmpb $0,3(%edi)
 je control_angles
 call control_bump2
control_angles:
 movb %al,1(%esi)
 movb (%esi),%al
 call control_cntr
 call control_cntr
 movb %al,(%esi)
 xorb $128,%al
 movb %al,%cl
 andb $128,%cl
 movb %cl,7(%esi)
 movb %cl,%bl
 xorb $128,%bl
 movb %bl,8(%esi)
 testb %al,%al
 jns control_roll_abs
 negb %al
control_roll_abs:
 andw $255,%ax
 shrw $2,%ax
 movl $1,%ebp
 cmpw $8,%ax
 jae control_roll_mag
 shrw $1,%ax
 movl $0,%ebp
control_roll_mag:
 movb %al,6(%esi)
 orb %cl,%al
 movb %al,4(%esi)
 xorl %eax,%eax
 movb 1(%esi),%al
 call control_cntr
 movb %al,1(%esi)
 xorb $128,%al
 movb %al,%cl
 andb $128,%cl
 movb %cl,11(%esi)
 xorb $128,%cl
 movb %cl,10(%esi)
 testb %al,%al
 jns control_pitch_abs
 notb %al
control_pitch_abs:
 addw $4,%ax
 movl %ebp,%edx
 addw %dx,%ax
 shrw $4,%ax
 cmpw $3,%ax
 jae control_pitch_mag
 shrw $1,%ax
control_pitch_mag:
 movb %al,9(%esi)
 orb %cl,%al
 movb %al,5(%esi)
 cmpb $0,4(%edi)
 je control_slower
 cmpb $40,12(%esi)
 jae control_slower
 addb $1,12(%esi)
control_slower:
 cmpb $0,5(%edi)
 je control_done
 subb $1,12(%esi)
 jne control_done
 addb $1,12(%esi)
control_done:
 popl %ebp
 popl %edi
 popl %esi
 popl %ebx
 ret
control_cntr:
 cmpb $0,2(%esi)
 jne control_return
 cmpw $128,%ax
 je control_return
 ja control_cntr_down
 addw $1,%ax
 ret
control_cntr_down:
 subw $1,%ax
 ret
control_bump2:
 addw %dx,%ax
 cmpw $255,%ax
 jbe control_bump_range
 movw $255,%ax
control_bump_range:
 cmpw $128,%ax
 jae control_return
 jmp control_recentre
control_redu2:
 subw %dx,%ax
 jns control_redu_range
 movl $1,%eax
control_redu_range:
 cmpw $128,%ax
 jb control_return
control_recentre:
 cmpb $0,3(%esi)
 jne control_return
 movw $128,%ax
control_return:
 ret
