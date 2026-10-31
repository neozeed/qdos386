# RC6 SOLAR/ZINF/SOS1 and original long-range chart drawing.
.text
.globl _solar_planet
_solar_planet:
 pushl %edi
 movl 8(%esp),%edi
 xorl %eax,%eax
 movl $36,%ecx
 cld
 rep
 stosb
 movl 8(%esp),%edi
 movb $96,18(%edi)
 movb $96,22(%edi)
 movb $224,14(%edi)
 movl 20(%esp),%edx
 andl $1,%edx
 movl 12(%esp),%ecx
 movzbl 1(%ecx),%eax
 andl $7,%eax
 leal 6(%eax,%edx),%eax
 movl %eax,%edx
 shrl $1,%eax
 movb %al,8(%edi)
 shrl $1,%eax
 andl $1,%edx
 shll $7,%edx
 orl %edx,%eax
 movb %al,2(%edi)
 movb %al,5(%edi)
 movb $127,29(%edi)
 movb $127,30(%edi)
 movl 16(%esp),%eax
 andl $2,%eax
 orl $128,%eax
 popl %edi
 ret
.globl _galaxy_chart
_galaxy_chart:
 movl 8(%esp),%eax
 subl $4,%esp
 movb 43(%eax),%dl
 movb %dl,(%esp)
 movb 41(%eax),%dl
 movb %dl,1(%esp)
 movl %esp,%edx
 pushl %edx
 pushl %eax
 pushl 16(%esp)
 call _galaxy_chart_cursor
 addl $16,%esp
 ret
.globl _galaxy_chart_cursor
_galaxy_chart_cursor:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $172,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 movl (%edi),%eax
 movl %eax,-184(%ebp)
 movw 4(%edi),%ax
 movw %ax,-180(%ebp)
 pushl $2
 movzbl 9(%edi),%eax
 shrl $2,%eax
 pushl %eax
 movzbl 17(%edi),%eax
 shrl $1,%eax
 addl $24,%eax
 pushl %eax
 movzbl 19(%edi),%eax
 pushl %eax
 leal -160(%ebp),%eax
 pushl %eax
 pushl %esi
 call _circle_xy
 addl $24,%esp
 movzbl 19(%edi),%eax
 movzbl 17(%edi),%edx
 movl $7,%ecx
 call chart_cross
 movl $0,-16(%ebp)
chart_system:
 movzbl -181(%ebp),%eax
 movzbl -183(%ebp),%edx
 shrl $1,%edx
 addl $24,%edx
 testb $7,%al
 jz chart_dash
 decl %eax
chart_dash:
 leal 1(%eax),%ebx
 movl %edx,%ecx
 call xor_segment
 leal -184(%ebp),%eax
 pushl %eax
 pushl $1
 pushl %eax
 call _galaxy_system
 addl $12,%esp
 incl -16(%ebp)
 cmpl $256,-16(%ebp)
 jb chart_system
 movl 16(%ebp),%ecx
 movzbl (%ecx),%eax
 movzbl 1(%ecx),%edx
 movl $4,%ecx
 call chart_cross
 addl $172,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _chart_cursor_xor
_chart_cursor_xor:
 pushl %ebx
 pushl %esi
 movl 12(%esp),%esi
 movl 16(%esp),%ecx
 movzbl (%ecx),%eax
 movzbl 1(%ecx),%edx
 movl $4,%ecx
 call chart_cross
 popl %esi
 popl %ebx
 ret
# EAX galactic X, EDX galactic Y, ECX size, ESI framebuffer.
chart_cross:
 pushl %eax
 pushl %ecx
 shrl $1,%edx
 addl $24,%edx
 movl %eax,%ebx
 subl %ecx,%eax
 jns chart_cross_left
 xorl %eax,%eax
chart_cross_left:
 addl %ecx,%ebx
 cmpl $255,%ebx
 jbe chart_cross_right
 movl $255,%ebx
chart_cross_right:
 pushl %edx
 movl %edx,%ecx
 call xor_segment
 popl %edx
 popl %ecx
 popl %eax
 movl %eax,%ebx
 pushl %ecx
 movl %edx,%eax
 addl %ecx,%eax
 subl %ecx,%edx
 cmpl $24,%edx
 jae chart_cross_top
 movl $24,%edx
chart_cross_top:
 cmpl $151,%eax
 jbe chart_cross_bottom
 movl $151,%eax
chart_cross_bottom:
 movl %eax,%ecx
 movl %ebx,%eax
 call xor_segment
 addl $4,%esp
 ret
