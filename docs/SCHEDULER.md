# Scheduling and CPU responsibilities

> Preserved R3 baseline document. For current R4 resource changes and profile status, see [R4 configuration](R4-CONFIGURATION.md), [ABI review](R4-ABI-REVIEW.md) and [known limitations](R4-KNOWN-LIMITATIONS.md). The R3 text below is retained for provenance.

The current scheduler is in jobs.c with policy/topology in smp17.c and entry frames in kctx17.s/smp_s17.s. It uses the existing priority-increment/accumulator model, not a new fair-share or Linux scheduler. Tagged slot IDs, suspend timeout/hold semantics, active/inactive priority and exit recording are unchanged.

CPU0's timer increments shared ticks, acknowledges the PIC timer, runs poll callbacks and the scheduling pass. AP LAPIC timer paths schedule locally without duplicating shared timekeeping/poll work. Kernel state transitions are under the recursive BKL with IF clear. A scheduling pass saves the outgoing frame/format and selects an eligible job for the CPU; a job already running elsewhere is not simultaneously selected. CR3/kernel-stack state is installed before resuming its saved context.

Foreground-first policy and HT/topology decisions remain as implemented: eligible foreground work prefers CPU0; AUTO background placement considers allowed background CPUs, load and topology rank; explicit affinity is respected. Parked siblings execute idle. The older two-CPU lending policy is not offered by the eight-context backend. CPU discovery can inspect more candidates than the fixed eight execution contexts; failed starts are not silently counted online.

BSP/AP idle jobs and IOW0 use the same 24-slot allocator as other jobs. There is no reserved 16 ordinary plus 8 AP partition. Idle is the fallback rather than a competitor that can outbid ordinary runnable work. More online CPUs consume more idle slots; advertised table capacity is not an actual-free-process guarantee.

Sleeping/restart changes status and schedules; input/worker wake callbacks mark runnable state without scheduling inside arbitrary IRQ callbacks. Reclamation is deferred until another trap/yield context can close channels, tear down private pages, return stacks/owner allocations, drop session state and notify cache/request cleanup. Do not free a victim's currently executing stack or CR3. Timer context does not perform ordinary heap reclamation.

Current bounds/policies are fixed by CONFIGURATION. R3 does not expand CPUs/jobs, alter priorities/affinity, add a worker facility, repair high-slot consoles or redesign the scheduler. ABI/register tests, fault/kill/reclaim and 1/8-CPU workload evidence are separate from hardware/soak qualification.
