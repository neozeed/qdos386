# Original DIALS and SHD. ESI screen; bar EAX length, EDX byte offset.
.text
.globl _energy_step
_energy_step:
 movl 4(%esp),%ecx
 testb $7,8(%esp)
 jnz energy_cool
 cmpb $0,2(%ecx)
 jns energy_charge
 cmpb $255,1(%ecx)
 je energy_front
 incb 1(%ecx)
 call energy_drain
energy_front:
 cmpb $255,(%ecx)
 je energy_charge
 incb (%ecx)
 call energy_drain
energy_charge:
 movzbl 2(%ecx),%eax
 movzbl 4(%ecx),%edx
 leal 1(%eax,%edx),%eax
 cmpl $255,%eax
 ja energy_cool
 movb %al,2(%ecx)
energy_cool:
 cmpb $0,3(%ecx)
 je energy_done
 decb 3(%ecx)
energy_done:
 ret
energy_drain:
 decb 2(%ecx)
 jnz energy_done
 incb 2(%ecx)
 ret
.globl _dials_frame
_dials_frame:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $4,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 movzbl 12(%edi),%eax
 movl $24936,%edx
 call dials_bar
 movzbl 6(%edi),%eax
 shrb $2,%al
 orb 7(%edi),%al
 xorb $128,%al
 call dials_add8
 movl $25832,%edx
 call dials_marker
 movzbl 5(%edi),%eax
 cmpb $0,9(%edi)
 je dials_pitch_zero
 subb $2,%al
 movzbl %al,%eax
dials_pitch_zero:
 call dials_add8
 movl $26856,%edx
 call dials_marker
 testb $3,20(%ebp)
 jnz dials_done
 movl 16(%ebp),%edi
 movl (%edi),%eax
 movl %eax,_dials_last_status
 movb 4(%edi),%al
 movb %al,_dials_last_status+4
 movl $0,-16(%ebp)
dials_energy_loop:
 movzbl 2(%edi),%eax
 shrl $1,%eax
 movl $3,%edx
 subl -16(%ebp),%edx
 shll $5,%edx
 subl %edx,%eax
 jns dials_energy_value
 xorl %eax,%eax
dials_energy_value:
 movl -16(%ebp),%edx
 shll $10,%edx
 addl $28008,%edx
 call dials_bar
 incl -16(%ebp)
 cmpl $4,-16(%ebp)
 jl dials_energy_loop
 movzbl (%edi),%eax
 shrl $3,%eax
 movl $24840,%edx
 call dials_bar
 movzbl 1(%edi),%eax
 shrl $3,%eax
 movl $25864,%edx
 call dials_bar
 movzbl 3(%edi),%eax
 shrl $3,%eax
 movl $28936,%edx
 call dials_bar
dials_done:
 addl $4,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
dials_add8:
 testb %al,%al
 js dials_add_negative
 addb $8,%al
 movzbl %al,%eax
 ret
dials_add_negative:
 andl $127,%eax
 subl $8,%eax
 jns dials_add_sign
 negl %eax
 ret
dials_add_sign:
 orl $128,%eax
 ret
dials_bar:
 pushl %edi
 leal (%esi,%edx),%edi
 movl $8,%ecx
dials_bar_loop:
 movl %eax,%edx
 cmpl $4,%edx
 jbe dials_bar_partial
 movl $4,%edx
dials_bar_partial:
 subl %edx,%eax
 movw dials_masks(,%edx,2),%bx
 movw %bx,(%edi)
 movw %bx,128(%edi)
 movw %bx,256(%edi)
 addl $2,%edi
 decl %ecx
 jnz dials_bar_loop
 popl %edi
 ret
dials_masks: .word 0,0xc080,0xf0a0,0xfca8,0xffaa
dials_marker:
 pushl %edi
 leal (%esi,%edx),%edi
 movl %edi,%edx
 movl $8,%ecx
dials_marker_clear:
 movw $0,(%edi)
 movw $0,128(%edi)
 movw $0,256(%edi)
 movw $0,384(%edi)
 addl $2,%edi
 decl %ecx
 jnz dials_marker_clear
 cmpl $15,%eax
 ja dials_marker_done
 movl %eax,%ecx
 andl $-2,%ecx
 addl %ecx,%edx
 movw $0xf0a0,%bx
 testb $1,%al
 jz dials_marker_write
 movw $0x0f0a,%bx
dials_marker_write:
 movw %bx,(%edx)
 movw %bx,128(%edx)
 movw %bx,256(%edx)
 movw %bx,384(%edx)
dials_marker_done:
 popl %edi
 ret
.globl _dials_fuel
_dials_fuel:
 pushl %esi
 pushl %ebx
 movl 12(%esp),%esi
 movzbl 16(%esp),%eax
 shrl $1,%eax
 movl $26888,%edx
 call dials_bar
 popl %ebx
 popl %esi
 ret
.globl _dials_altitude
_dials_altitude:
 pushl %esi
 pushl %ebx
 movl 12(%esp),%esi
 movzbl 16(%esp),%eax
 shrl $3,%eax
 movl $29960,%edx
 call dials_bar
 popl %ebx
 popl %esi
 ret
.data
.align 2
.globl _dials_last_status
_dials_last_status: .space 6
