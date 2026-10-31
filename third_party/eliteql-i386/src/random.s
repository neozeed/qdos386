# Extracted public DORND entry from RC6 combat.s, lines 17-51.
# Returns the original packed A/carry/X, not just the low random byte.
.text
.globl _elite_random
_elite_random:
 pushl %ebx
 pushl %esi
 movl 12(%esp),%esi
 movl 16(%esp),%edx
 andl $1,%edx
 movzbl (%esi),%eax
 movl %eax,%ecx
 shrl $7,%ecx
 addl %eax,%eax
 orl %edx,%eax
 andl $255,%eax
 movl %eax,%ebx
 movzbl 2(%esi),%edx
 addl %edx,%eax
 addl %ecx,%eax
 movb %al,(%esi)
 movb %bl,2(%esi)
 shrl $8,%eax
 movzbl 1(%esi),%ecx
 addl %ecx,%eax
 movzbl 3(%esi),%edx
 addl %edx,%eax
 movb %al,1(%esi)
 movb %cl,3(%esi)
 shll $16,%ecx
 orl %ecx,%eax
 popl %esi
 popl %ebx
 ret
