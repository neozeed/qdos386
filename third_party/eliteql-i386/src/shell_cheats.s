# Exact original cheat actions; callable native labels, no new actions.
.text
.globl cheat_actions
cheat_actions:
 .word cheat_cash-cheat_actions,cheat_missiles-cheat_actions,cheat_computer-cheat_actions,cheat_bomb-cheat_actions,cheat_ecm-cheat_actions,cheat_drive-cheat_actions
.globl cheat_cash
cheat_cash:
 movl asm_commander,%eax
 xchgb %al,%ah
 roll $16,%eax
 xchgb %al,%ah
 addl $10000,%eax
 jnc cheat_cash_done
 movl $-1,%eax
cheat_cash_done:
 xchgb %al,%ah
 roll $16,%eax
 xchgb %al,%ah
 movl %eax,asm_commander
 ret
.globl cheat_missiles
cheat_missiles:
 movb $4,asm_player
 ret
.globl cheat_computer
cheat_computer:
 movb $255,asm_equipment+4
 ret
.globl cheat_bomb
cheat_bomb:
 testb $128,asm_equipment+2
 jnz cheat_bomb_done
 movb $127,asm_equipment+2
cheat_bomb_done:
 ret
.globl cheat_ecm
cheat_ecm:
 movb $255,asm_equipment
 ret
.globl cheat_drive
cheat_drive:
 movb $255,asm_travel+10
 ret
