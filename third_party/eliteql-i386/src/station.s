# RC6 station.s: candidate placement, launch and two-body WARP gates.
.text
.globl _station_candidate
_station_candidate:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $8,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 movb 2(%edi),%al
 orb 5(%edi),%al
 orb 8(%edi),%al
 andb $127,%al
 jne station_no
 movl $35,%ecx
station_clear:
 movb $0,(%esi,%ecx)
 decl %ecx
 jns station_clear
 movl $28,%ecx
station_copy:
 movb (%edi,%ecx),%al
 movb %al,(%esi,%ecx)
 decl %ecx
 jns station_copy
 leal 9(%esi),%edi
 movl $2,-16(%ebp)
station_axis:
 movzwl (%edi),%eax
 movl %eax,%ebx
 andl $0x8000,%ebx
 shll $8,%ebx
 andl $0x7fff,%eax
 addl %eax,%eax
 movzbl 2(%esi),%edx
 shll $8,%edx
 movb 1(%esi),%dl
 shll $8,%edx
 movb (%esi),%dl
 movl %edx,%ecx
 andl $0x800000,%ecx
 andl $0x7fffff,%edx
 cmpl %ebx,%ecx
 jne station_sub
 addl %edx,%eax
 andl $0x7fffff,%eax
 orl %ebx,%eax
 jmp station_store
station_sub:
 subl %eax,%edx
 jnc station_coord_larger
 negl %edx
 movl %ebx,%ecx
station_coord_larger:
 movl %edx,%eax
 orl %ecx,%eax
station_store:
 movb %al,(%esi)
 shrl $8,%eax
 movb %al,1(%esi)
 shrl $8,%eax
 movb %al,2(%esi)
 andb $127,%al
 jne station_no
 addl $3,%esi
 addl $2,%edi
 subl $1,-16(%ebp)
 jns station_axis
 movl 8(%ebp),%esi
 cmpb $192,1(%esi)
 ja station_no
 cmpb $192,4(%esi)
 ja station_no
 cmpb $192,7(%esi)
 ja station_no
 movb $1,32(%esi)
 movb $0,30(%esi)
 movb $255,29(%esi)
 xorb $128,10(%esi)
 xorb $128,12(%esi)
 xorb $128,14(%esi)
 movl $1,%eax
 jmp station_return
station_no:
 xorl %eax,%eax
station_return:
 addl $8,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _station_launch
_station_launch:
 movl 4(%esp),%edx
 movl 8(%esp),%ecx
 movl $35,%eax
launch_world_zero:
 movb $0,(%edx,%eax)
 movb $0,(%ecx,%eax)
 decl %eax
 jns launch_world_zero
 movb $224,14(%edx)
 movb $96,18(%edx)
 movb $96,22(%edx)
 movb $1,8(%edx)
 movb $127,29(%edx)
 movb $127,30(%edx)
 movb $96,14(%ecx)
 movb $96,18(%ecx)
 movb $96,22(%ecx)
 movb $128,10(%ecx)
 movb $128,12(%ecx)
 movb $128,8(%ecx)
 movb $1,7(%ecx)
 movb $255,29(%ecx)
 movb $1,32(%ecx)
 ret
.globl _warp_planet
_warp_planet:
 movl 4(%esp),%edx
 cmpl $0,8(%esp)
 jne warp_no
 cmpb $0,8(%edx)
 js warp_go
 movb 2(%edx),%al
 orb 5(%edx),%al
 orb 8(%edx),%al
 andb $127,%al
 cmpb $2,%al
 jb warp_no
warp_go:
 movb 8(%edx),%al
 testb %al,%al
 js warp_negative
 je warp_cross
 subb $1,%al
 jmp warp_store
warp_cross:
 movb $129,%al
 jmp warp_store
warp_negative:
 addb $2,%al
 orb $128,%al
warp_store:
 movb %al,8(%edx)
 movl $1,%eax
 ret
warp_no:
 xorl %eax,%eax
 ret
.globl _warp_world
_warp_world:
 pushl %esi
 pushl %edi
 movl 12(%esp),%esi
 movl 16(%esp),%edi
 cmpl $0,20(%esp)
 jne warp_world_no
 movl %esi,%edx
 call warp_distance
 testl %eax,%eax
 je warp_world_no
 movl %edi,%edx
 call warp_distance
 testl %eax,%eax
 je warp_world_no
 pushl $0
 pushl %esi
 call _warp_planet
 addl $8,%esp
 pushl $0
 pushl %edi
 call _warp_planet
 addl $8,%esp
 movl $1,%eax
 jmp warp_world_return
warp_world_no:
 xorl %eax,%eax
warp_world_return:
 popl %edi
 popl %esi
 ret
warp_distance:
 cmpb $0,8(%edx)
 js warp_distance_yes
 movb 2(%edx),%al
 orb 5(%edx),%al
 orb 8(%edx),%al
 andb $127,%al
 cmpb $2,%al
 jb warp_no
warp_distance_yes:
 movl $1,%eax
 ret
