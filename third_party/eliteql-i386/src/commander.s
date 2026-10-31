# RC6 commander.s. CASH remains big endian; TALLY remains little endian.
# Byte swaps use 386 rotations, never the 486 BSWAP instruction.
.text
.globl _commander_tally
_commander_tally:
 movl 4(%esp),%edx
 addb $1,4(%edx)
 jne commander_tally_done
 addb $1,5(%edx)
 movb $22,7(%edx)
 movb $2,8(%edx)
commander_tally_done:
 ret
.globl _commander_settle
_commander_settle:
 movl 4(%esp),%edx
 movl 8(%esp),%eax
 cmpb $2,%al
 jne commander_bounty
 orb $64,6(%edx)
commander_bounty:
 cmpb $0,7(%edx)
 jne commander_no_cash
 movl 12(%esp),%ecx
 testb %cl,%cl
 je commander_no_cash
 andl $65535,%ecx
 movl (%edx),%eax
 xchgb %al,%ah
 roll $16,%eax
 xchgb %al,%ah
 addl %ecx,%eax
 xchgb %al,%ah
 roll $16,%eax
 xchgb %al,%ah
 movl %eax,(%edx)
 movb $22,7(%edx)
 movb $1,8(%edx)
 movl $1,%eax
 ret
commander_no_cash:
 xorl %eax,%eax
 ret
.globl _commander_rank
_commander_rank:
 movl 4(%esp),%edx
 movzbl 5(%edx),%eax
 testb %al,%al
 je commander_low_rank
 movl $9,%ecx
 cmpb $25,%al
 jae commander_rank_done
 decl %ecx
 cmpb $10,%al
 jae commander_rank_done
 decl %ecx
 cmpb $2,%al
 jae commander_rank_done
 decl %ecx
 jmp commander_rank_done
commander_low_rank:
 movb 4(%edx),%al
 shrb $2,%al
 xorl %ecx,%ecx
commander_rank_loop:
 incl %ecx
 shrb $1,%al
 jne commander_rank_loop
commander_rank_done:
 movl %ecx,%eax
 ret
.globl _commander_tick
_commander_tick:
 movl 4(%esp),%edx
 subb $1,7(%edx)
 je commander_tick_done
 jns commander_tick_done
 addb $1,7(%edx)
commander_tick_done:
 ret
