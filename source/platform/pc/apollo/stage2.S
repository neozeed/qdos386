/* Apollo R6 bounded FAT16 package loader; EDD-only, linked at0x1000. */
.code16
.section .text
.global start
start:
 jmp loader_entry
.org 8
 .ascii "R6L1"
 .long 16384
 .long 0 /* builder patches additive checksum over complete16KiB stage */
.org 24
loader_entry:
 cld
 xorw %ax,%ax
 movw %ax,%ds
 movw %ax,%es
 movb %dl,drive
 movw %si,entry_ptr
 /* Capture relocated MBR identity before E820 overwrites0x600. */
 movl $0x49423652,0x900
 movl $1,0x904
 movl $48,0x908
 movzbl %dl,%eax
 movl %eax,0x90c
 movzwl %si,%eax
 subl $0x7be,%eax
 testl $15,%eax
 jnz bad_p
 shrl $4,%eax
 cmpl $4,%eax
 jae bad_p
 movl %eax,0x910
 movl 8(%si),%eax
 movl %eax,part_start
 movl %eax,0x914
 movl 12(%si),%eax
 movl %eax,part_count
 movl %eax,0x918
 movl 0x7b8,%eax
 movl %eax,0x91c
 testl %eax,%eax
 jz bad_p
 movl $0,0x920
 movzbl 4(%si),%eax
 movl %eax,0x924
 movw $30,0xa00
 movw $0xa00,%si
 movb drive,%dl
 movw $0x4800,%ax
 int $0x13
 jc bad_r
 cmpw $512,0xa18
 jne bad_p
 cmpl $0,0xa14
 jne bad_p
 movl 0xa10,%eax
 testl %eax,%eax
 jz bad_p
 cmpl $0x10000000,%eax
 ja bad_p
 movl %eax,dev_count
 movl %eax,0x928
 movl part_start,%ecx
 addl part_count,%ecx
 jc bad_p
 cmpl %eax,%ecx
 ja bad_p
 cmpw $512,0x413
 jb bad_m
 movzwl 0x40e,%eax
 shll $4,%eax
 testl %eax,%eax
 jz memory_ok
 cmpl $0x80000,%eax
 jb bad_m
memory_ok:
 /* FAT16 BPB validation from the VBR already at7c00. */
 cmpw $512,0x7c0b
 jne bad_b
 movzbl 0x7c0d,%eax
 testl %eax,%eax
 jz bad_b
 cmpl $128,%eax
 ja bad_b
 movl %eax,%ecx
 decl %ecx
 testl %eax,%ecx
 jnz bad_b
 movl %eax,spc
 cmpb $2,0x7c10
 jne bad_b
 movzwl 0x7c0e,%eax
 testl %eax,%eax
 jz bad_b
 movl %eax,fat_start
 movzwl 0x7c16,%eax
 testl %eax,%eax
 jz bad_b
 movl %eax,fat_sectors
 shll $1,%eax
 addl fat_start,%eax
 movl %eax,root_start
 movzwl 0x7c11,%eax
 testl %eax,%eax
 jz bad_b
 testl $15,%eax
 jnz bad_b
 shrl $4,%eax
 movl %eax,root_sectors
 addl root_start,%eax
 movl %eax,data_start
 movl 0x7c1c,%eax
 cmpl part_start,%eax
 jne bad_b
 movzwl 0x7c13,%eax
 testl %eax,%eax
 jnz total_ok
 movl 0x7c20,%eax
total_ok:
 cmpl part_count,%eax
 ja bad_b
 cmpl data_start,%eax
 jbe bad_b
 movl %eax,vol_count
 subl data_start,%eax
 xorl %edx,%edx
 divl spc
 cmpl $4085,%eax
 jb bad_b
 cmpl $65524,%eax
 ja bad_b
 addl $1,%eax
 movl %eax,max_cluster
 incl %eax
 movl fat_sectors,%ecx
 shll $8,%ecx
 cmpl %eax,%ecx
 jb bad_b
 /* Root is bounded by BPB root sectors. */
 movl root_start,%ebp
 movl %ebp,root_sector
root_loop:
 movl %ebp,%eax
 call read_relative
 movw $0x8000,%di
 movw $16,%bp
entry_loop:
 cmpb $0,(%di)
 je bad_k
 testb $0x18,11(%di)
 jnz skip_entry
 pushw %di
 pushw %si
 pushw %cx
 movw $kernel_name,%si
 movw $11,%cx
 repe cmpsb
 popw %cx
 popw %si
 popw %di
 je found
skip_entry:
 addw $32,%di
 decw %bp
 jnz entry_loop
 movl root_sector,%ebp
 incl %ebp
 movl %ebp,root_sector
 movl root_start,%eax
 addl root_sectors,%eax
 cmpl %eax,%ebp
 jb root_loop
 jmp bad_k
found:
 movl 28(%di),%eax
 testl $511,%eax
 jnz bad_k
 cmpl $1024,%eax
 jb bad_k
 cmpl $(897*512),%eax
 ja bad_k
 shrl $9,%eax
 movl %eax,file_sectors
 movl %eax,left
 movzwl 26(%di),%eax
 movl %eax,cluster
 movw $0x1000,dest_seg
 movb $1,header_pending
file_cluster:
 movl cluster,%eax
 cmpl $2,%eax
 jb bad_c
 cmpl max_cluster,%eax
 ja bad_c
 subl $2,%eax
 mull spc
 testl %edx,%edx
 jnz bad_c
 addl data_start,%eax
 movl %eax,cluster_lba
 movl $0,sector_in_cluster
file_sector:
 movl cluster_lba,%eax
 addl sector_in_cluster,%eax
 call read_relative
 cmpb $0,header_pending
 je copy_payload
 cmpw $0xaa55,0x81fe
 jne bad_k
 cmpl $0x4f4f4251,0x81ca
 jne bad_k
 cmpw $0x3354,0x81ce
 jne bad_k
 movzwl 0x81c2,%eax
 testl %eax,%eax
 jz bad_k
 movl %eax,ksec
 movzwl 0x81c4,%ecx
 testl %ecx,%ecx
 jz bad_k
 cmpl $512,%ecx
 ja bad_k
 movl %ecx,xsec
 addl %ecx,%eax
 cmpl $896,%eax
 ja bad_k
 incl %eax
 cmpl file_sectors,%eax
 jne bad_k
 movl 0x81c6,%eax
 movl %eax,expected_sum
 movb $0,header_pending
 jmp copied
copy_payload:
 movw dest_seg,%ax
 movw %ax,%es
 xorw %di,%di
 movw $0x8000,%si
 movw $256,%cx
 rep movsw
 addw $32,dest_seg
 xorw %ax,%ax
 movw %ax,%es
copied:
 decl left
 incl sector_in_cluster
 cmpl $0,left
 je last_cluster
 movl sector_in_cluster,%eax
 cmpl spc,%eax
 jb file_sector
 call next_cluster
 movl %eax,cluster
 jmp file_cluster
last_cluster:
 call next_cluster
 cmpl $0xfff8,%eax
 jb bad_c
 /* E820: same bounded layout as QBOOT3, after all relocated MBR use. */
 xorl %ebx,%ebx
 movw $0x504,%di
e820_loop:
 movl $0xe820,%eax
 movl $0x534d4150,%edx
 movl $24,%ecx
 int $0x15
 jc e820_done
 cmpl $0x534d4150,%eax
 jne e820_done
 addw $24,%di
 cmpw $0x804,%di
 jae e820_done
 testl %ebx,%ebx
 jnz e820_loop
e820_done:
 movw %di,0x500
 cmpw $0x504,%di
 je no_e820
 movl $1,0x920
no_e820:
 xorl %eax,%eax
 movw $0x900,%si
 movw $11,%cx
info_sum:
 addl (%si),%eax
 addw $4,%si
 loop info_sum
 negl %eax
 movl %eax,0x92c
 cli
 inb $0x92,%al
 orb $2,%al
 andb $0xfe,%al
 outb %al,$0x92
 lgdt gdtdesc
 movl %cr0,%eax
 orb $1,%al
 movl %eax,%cr0
 ljmpl $8,$protected
next_cluster:
 movl cluster,%eax
 shll $1,%eax
 movl %eax,%ebx
 andl $511,%ebx
 shrl $9,%eax
 cmpl fat_sectors,%eax
 jae bad_c
 addl fat_start,%eax
 call read_relative
 movzwl 0x8000(%ebx),%eax
 ret
read_relative:
 cmpl vol_count,%eax
 jae bad_p
 movl %eax,root_sector_saved
 addl part_start,%eax
 jc bad_p
 call read_absolute
 ret
read_absolute:
 cmpl dev_count,%eax
 jae bad_p
 movl %eax,dap+8
 pushal
 movw $dap,%si
 movw $1,dap+2
 movb drive,%dl
 movw $0x4200,%ax
 int $0x13
 jc bad_r
 testb %ah,%ah
 jnz bad_r
 cmpw $1,dap+2
 jne bad_r
 popal
 ret
bad_p: movb $'P',%al; jmp fail
bad_b: movb $'B',%al; jmp fail
bad_k: movb $'K',%al; jmp fail
bad_c: movb $'C',%al; jmp fail
bad_r: movb $'R',%al; jmp fail
bad_m: movb $'M',%al
fail:
 movb %al,errcode
 movw $message,%si
print:
 lodsb
 testb %al,%al
 jz err
 movw $0x402,%dx
 outb %al,%dx
 jmp print
err:
 movb errcode,%al
 movw $0x402,%dx
 outb %al,%dx
 movb $10,%al
 outb %al,%dx
 cli
halt: hlt; jmp halt
.code32
protected:
 movw $16,%ax
 movw %ax,%ds
 movw %ax,%es
 movw %ax,%ss
 movw %ax,%fs
 movw %ax,%gs
 movl $0x90000,%esp
 cld
 movl 0x500,%eax
 notl %eax
 movl %eax,0x100500
 cmpl 0x500,%eax
 je pm_a20
 movl ksec,%ecx
 addl xsec,%ecx
 shll $7,%ecx
 movl $0x10000,%esi
 xorl %ebx,%ebx
sum_loop:
 lodsl
 addl %eax,%ebx
 roll $1,%ebx
 loop sum_loop
 cmpl expected_sum,%ebx
 jne pm_sum
 movl $0x10000,%esi
 movl $0x100000,%edi
 movl ksec,%ecx
 shll $7,%ecx
 rep movsl
 movl $0x340000,%edi
 movl xsec,%ecx
 shll $7,%ecx
 rep movsl
 movl $0x900,%ebx
 movl $0x100000,%eax
 jmp *%eax
pm_a20: movb $'A',%al; jmp pm_fail
pm_sum: movb $'S',%al
pm_fail:
 movw $0x402,%dx
 outb %al,%dx
 cli
pm_halt: hlt; jmp pm_halt
.balign 8
gdt: .quad 0,0x00cf9a000000ffff,0x00cf92000000ffff
gdtdesc: .word 23; .long gdt
.balign 4
dap: .byte 16,0; .word 1,0x8000,0; .long 0,0
kernel_name: .ascii "QDOS    SYS"
message: .asciz "R6LOAD:"
drive: .byte 0
errcode: .byte 0
header_pending: .byte 0
entry_ptr: .word 0
dest_seg: .word 0
.balign 4
part_start: .long 0
part_count: .long 0
dev_count: .long 0
spc: .long 0
fat_start: .long 0
fat_sectors: .long 0
root_start: .long 0
root_sectors: .long 0
root_sector: .long 0
root_sector_saved: .long 0
data_start: .long 0
vol_count: .long 0
max_cluster: .long 0
file_sectors: .long 0
left: .long 0
cluster: .long 0
cluster_lba: .long 0
sector_in_cluster: .long 0
ksec: .long 0
xsec: .long 0
expected_sum: .long 0
