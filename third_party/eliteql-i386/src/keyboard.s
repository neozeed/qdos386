# RC6 queue drain, bounded at 128 events. No held-key matrix changes.
# Queue-pop returns nonzero for absent/empty queue or an external error.
.text
.globl _keyboard_drain
_keyboard_drain:
 pushl %ebx
 movl $128,%ebx
queue_next:
 call _platform_keyboard_queue_pop
 testl %eax,%eax
 jnz drain_done
 decl %ebx
 jnz queue_next
drain_done:
 popl %ebx
 ret
