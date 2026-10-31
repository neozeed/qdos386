# RC6 market.s: QQ23 generation and exact TT219/TT210/TT224 transactions.
.text
.globl _market_init
_market_init:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $4,%esp
 movl 8(%ebp),%edi
 movl $market_table,%esi
 xorl %ebx,%ebx
market_item:
 movzbl 1(%esi),%eax
 andl $31,%eax
 movzwl 12(%ebp),%edx
 imull %edx,%eax
 movl %eax,-16(%ebp)
 movzbl 3(%esi),%edx
 andb 16(%ebp),%dl
 movb (%esi),%al
 addb %dl,%al
 cmpb $0,1(%esi)
 js market_negative_price
 addb -16(%ebp),%al
 jmp market_price
market_negative_price:
 subb -16(%ebp),%al
market_price:
 movb %al,(%edi,%ebx)
 movb 2(%esi),%al
 addb %dl,%al
 cmpb $0,1(%esi)
 js market_negative_qty
 subb -16(%ebp),%al
 jmp market_qty
market_negative_qty:
 addb -16(%ebp),%al
market_qty:
 testb %al,%al
 jns market_qty_nonnegative
 movl $0,%eax
market_qty_nonnegative:
 andb $63,%al
 movb %al,17(%edi,%ebx)
 addl $4,%esi
 incl %ebx
 cmpl $17,%ebx
 jb market_item
 movb $0,33(%edi)
 addl $4,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _market_trade
_market_trade:
 xorl %eax,%eax
 jmp market_entry
.globl _market_buy
_market_buy:
 movl $1,%eax
market_entry:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $12,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 movl 16(%ebp),%edx
 movl %edx,-16(%ebp)
 movl 20(%ebp),%ebx
 testl %eax,%eax
 jne quantity_buy
 cmpw $17,%bx
 jae trade_stock
 andl $65535,%ebx
 movzbl (%edx,%ebx),%eax
 movl %eax,-24(%ebp)
 cmpb $0,24(%ebp)
 jne trade_sell
 cmpb $0,17(%edx,%ebx)
 je trade_stock
 movl $1,-20(%ebp)
 jmp trade_room
quantity_buy:
 cmpl $17,%ebx
 jae trade_stock
 movl 24(%ebp),%ecx
 cmpl $255,%ecx
 ja trade_stock
 cmpb 17(%edx,%ebx),%cl
 ja trade_stock
 movl %ecx,-20(%ebp)
 movzbl (%edx,%ebx),%eax
 movl %eax,-24(%ebp)
trade_room:
 movzbl _ship_fit,%eax
 pushl %eax
 pushl -20(%ebp)
 pushl %ebx
 pushl %edi
 call _cargo_room
 addl $16,%esp
 testl %eax,%eax
 je trade_capacity
 movl -24(%ebp),%eax
 imull -20(%ebp),%eax
 shlw $2,%ax
 andl $65535,%eax
 movl (%esi),%edx
 xchgb %dl,%dh
 roll $16,%edx
 xchgb %dl,%dh
 cmpl %edx,%eax
 ja trade_cash
 subl %eax,%edx
 xchgb %dl,%dh
 roll $16,%edx
 xchgb %dl,%dh
 movl %edx,(%esi)
 movl -20(%ebp),%eax
 addb %al,(%edi,%ebx)
 movl -16(%ebp),%edx
 subb %al,17(%edx,%ebx)
 jmp trade_success
trade_sell:
 movzbl (%edi,%ebx),%eax
 testb %al,%al
 je trade_stock
 imull -24(%ebp),%eax
 shlw $2,%ax
 andl $65535,%eax
 movl (%esi),%edx
 xchgb %dl,%dh
 roll $16,%edx
 xchgb %dl,%dh
 addl %eax,%edx
 xchgb %dl,%dh
 roll $16,%edx
 xchgb %dl,%dh
 movl %edx,(%esi)
 movb $0,(%edi,%ebx)
trade_success:
 xorl %eax,%eax
 jmp trade_done
trade_stock:
 movl $1,%eax
 jmp trade_done
trade_capacity:
 movl $2,%eax
 jmp trade_done
trade_cash:
 movl $3,%eax
trade_done:
 addl $12,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _launch_legal
_launch_legal:
 movl 4(%esp),%edx
 movl 8(%esp),%ecx
 movb 3(%ecx),%al
 addb 6(%ecx),%al
 movzbl %al,%eax
 movl %eax,%edx
 shrl $7,%edx
 addb %al,%al
 addb 10(%ecx),%al
 addb %dl,%al
 movl 4(%esp),%edx
 orb %al,6(%edx)
 ret
.data
market_table:
.byte 19,130,6,1,20,129,10,3,65,131,2,7,40,133,226,31
.byte 83,133,251,15,196,8,54,3,235,29,8,120,154,14,56,3
.byte 117,6,40,7,78,1,17,31,124,13,29,7,176,137,220,63
.byte 32,129,53,3,97,161,66,7,171,162,55,31,45,193,250,15,53,15,192,7
