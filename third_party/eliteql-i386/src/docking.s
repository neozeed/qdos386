# RC6 physical docking check. MA64 intent gate remains computer_dock.s.
.text
.globl _dock_check
_dock_check:
 pushl %ebp
 movl %esp,%ebp
 pushl %esi
 subl $4,%esp
 movl 8(%ebp),%esi
 movb 31(%esi),%al
 andb $160,%al
 orb 1(%esi),%al
 orb 4(%esi),%al
 orb 7(%esi),%al
 jne dock_far
 movb (%esi),%al
 orb 3(%esi),%al
 orb 6(%esi),%al
 js dock_far
 cmpb $0,32(%esi)
 js dock_bad
 cmpb $214,14(%esi)
 jb dock_bad
 leal -8(%ebp),%eax
 pushl %eax
 pushl %esi
 call _tactics_vector
 addl $8,%esp
 movb -6(%ebp),%al
 testb %al,%al
 js dock_bad
 cmpb $89,%al
 jb dock_bad
 movb 16(%esi),%al
 andb $127,%al
 cmpb $80,%al
 jb dock_bad
 movl $1,%eax
 jmp dock_done
dock_bad:
 movl $2,%eax
 cmpb $5,12(%ebp)
 jb dock_done
 movl $3,%eax
 jmp dock_done
dock_far:
 xorl %eax,%eax
dock_done:
 addl $4,%esp
 popl %esi
 popl %ebp
 ret
