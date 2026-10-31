# RC6 shop.s. CASH stays BE32; fitting flags and validation order retained.
.text
.globl _shop_buy
.globl _shop_buy_view
.globl _laser_fitting
_shop_buy:
 xorl %eax,%eax
 jmp shop_entry
_shop_buy_view:
 movl 24(%esp),%eax
shop_entry:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $12,%esp
 movl %eax,-16(%ebp)
 movl 8(%ebp),%esi
 movl 20(%ebp),%edi
 movl 24(%ebp),%ebx
 movl $4,%eax
 cmpw $12,%bx
 jae shop_done
 cmpw $4,-16(%ebp)
 jae shop_done
 movzbl 24(%edi),%edx
 addw $2,%dx
 movl $3,%eax
 cmpw %dx,%bx
 ja shop_done
 movzwl shop_prices(,%ebx,2),%edx
 testw %bx,%bx
 jne shop_price_ready
 movl $70,%edx
 subb 9(%edi),%dl
 addw %dx,%dx
shop_price_ready:
 movl %edx,-20(%ebp)
 movl (%esi),%ecx
 xchgb %cl,%ch
 roll $16,%ecx
 xchgb %cl,%ch
 movl %ecx,-24(%ebp)
 movl $1,%eax
 cmpl %ecx,%edx
 ja shop_done
 movl $2,%eax
 cmpw $1,%bx
 jne shop_bay
 movl 16(%ebp),%edx
 cmpb $4,(%edx)
 jae shop_done
 addb $1,(%edx)
 movb $0,1(%edx)
 movb $255,2(%edx)
 jmp shop_pay
shop_bay:
 cmpw $2,%bx
 jne shop_laser
 cmpb $37,_ship_fit
 je shop_done
 movb $37,_ship_fit
 jmp shop_pay
shop_laser:
 movl $_ship_fit+1,%edx
 movl -16(%ebp),%ecx
 testw %cx,%cx
 je shop_laser_pointer
 movl $_ship_fit+3,%edx
 movswl %cx,%ecx
 addl %ecx,%edx
shop_laser_pointer:
 cmpw $4,%bx
 je shop_pulse
 cmpw $5,%bx
 jne shop_flags
 cmpb $0,(%edx)
 js shop_done
 je shop_beam_fit
 addl $4000,-24(%ebp)
shop_beam_fit:
 movb $143,(%edx)
 jmp shop_pay
shop_pulse:
 cmpb $0,(%edx)
 jne shop_done
 movb $15,(%edx)
 jmp shop_pay
shop_flags:
 testw %bx,%bx
 jne shop_not_fuel
 movb $70,9(%edi)
 jmp shop_pay
shop_not_fuel:
 cmpw $7,%bx
 jne shop_not_escape
 movl $_ship_fit+2,%edx
 movl $-1,%ecx
 jmp shop_flag
shop_not_escape:
 cmpw $11,%bx
 jne shop_qq20
 leal 10(%edi),%edx
 movl $-1,%ecx
 jmp shop_flag
shop_qq20:
 movl 12(%ebp),%edx
 addl $17,%edx
 movl $-1,%ecx
 cmpw $3,%bx
 je shop_flag
 incl %edx
 cmpw $6,%bx
 je shop_flag
 incl %edx
 movl $127,%ecx
 cmpw $8,%bx
 je shop_flag
 incl %edx
 movl $1,%ecx
 cmpw $9,%bx
 je shop_flag
 incl %edx
 movl $-1,%ecx
shop_flag:
 cmpb $0,(%edx)
 jne shop_done
 movb %cl,(%edx)
shop_pay:
 movl -24(%ebp),%eax
 subl -20(%ebp),%eax
 xchgb %al,%ah
 roll $16,%eax
 xchgb %al,%ah
 movl %eax,(%esi)
 xorl %eax,%eax
shop_done:
 addl $12,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.data
shop_prices: .word 0,300,4000,6000,4000,10000,5250,10000,9000,15000,10000,50000
.text
.globl _escape_commander
_escape_commander:
 xorl %eax,%eax
 cmpb $0,_ship_fit+2
 je escape_done
 movl 4(%esp),%edx
 movb $0,6(%edx)
 movl 8(%esp),%edx
 movl $16,%ecx
escape_empty:
 movb $0,(%edx)
 incl %edx
 decl %ecx
 jns escape_empty
 movl 12(%esp),%edx
 movb $70,9(%edx)
 movb $0,11(%edx)
 movb $0,13(%edx)
 movb $0,_ship_fit+2
 movl $1,%eax
escape_done:
 ret
_laser_fitting:
 movl 4(%esp),%edx
 movl 8(%esp),%ecx
 xorl %eax,%eax
 cmpl $4,%ecx
 jae fitting_done
 testw %cx,%cx
 jne fitting_side
 movb 1(%edx),%al
 ret
fitting_side:
 movb 3(%edx,%ecx),%al
fitting_done:
 ret
