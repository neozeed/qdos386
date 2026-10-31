# RC6 SPBLB/ECBLB packed bitmap bytes, unchanged.
.text
.globl _dashboard_bulbs
_dashboard_bulbs:
 pushl %ebp
 movl %esp,%ebp
 pushl %esi
 pushl %edi
 movl 8(%ebp),%esi
 addl $29792,%esi
 movl $bulb_rows+12,%edi
 movl 12(%ebp),%eax
 call bulb_draw
 movl 8(%ebp),%esi
 addl $29724,%esi
 movl $bulb_rows,%edi
 movl 16(%ebp),%eax
 call bulb_draw
 popl %edi
 popl %esi
 popl %ebp
 ret
bulb_draw:
 movl $7,%ecx
bulb_row:
 movl (%edi),%edx
 addl $4,%edi
 testl %eax,%eax
 jne bulb_on
 movl $0,%edx
bulb_on:
 movl %edx,(%esi)
 addl $128,%esi
 decl %ecx
 jns bulb_row
 ret
.data
bulb_rows:
.byte 0xaa,0xff,0xa8,0xfc,0xaa,0xff,0xa8,0xfc,0xa8,0xfc,0,0
.byte 0xaa,0xff,0xa8,0xfc,0xaa,0xff,0xa8,0xfc,0xa8,0xfc,0,0
.byte 0xaa,0xff,0xa8,0xfc,0xaa,0xff,0xa8,0xfc,0,0,0xa8,0xfc
.byte 0xaa,0xff,0xa8,0xfc,0xaa,0xff,0xa8,0xfc
