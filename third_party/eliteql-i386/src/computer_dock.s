# RC6 docking.s public MA64 gate only. _dock_check is not ported here.
.text
.globl _computer_dock
_computer_dock:
 movl 4(%esp),%eax
 andl 8(%esp),%eax
 andl 12(%esp),%eax
 andl $255,%eax
 je computer_no
 movl 16(%esp),%edx
 testb %dl,%dl
 js computer_no
 movl $1,%eax
 ret
computer_no:
 xorl %eax,%eax
 ret
