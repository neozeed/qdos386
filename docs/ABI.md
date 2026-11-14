# Native ABI contract

> Preserved R3 baseline document. For current R4 resource changes and profile status, see [R4 configuration](R4-CONFIGURATION.md), [ABI review](R4-ABI-REVIEW.md) and [known limitations](R4-KNOWN-LIMITATIONS.md). The R3 text below is retained for provenance.

Authoritative interfaces are `source/kernel/jobs.h`, proc.h, aout.h, channel.h, surface.h, smp.h and the unchanged native `sdk/include/qdos386.h`. `source/kernel/kctx17.s` and `sdk/lib/qtrap.s` implement the actual boundary. This document describes the current native i386 ABI, not the 68000 QDOS register ABI.

- Service entry is `int 0x30`: EAX identifies the function; EBX/ECX/EDX carry arguments; EAX returns the result. Existing trap numbers and suboperations are unchanged. Internal worker traps 0x40–0x45 remain internal diagnostics, not a new public API.
- Stubs save the register/CPU frame with pushal. Trap dispatch updates the saved EAX result; return restores the saved registers and privilege frame. Callee-saved C registers and SDK entry/unwind rules remain as implemented. Do not infer an SSE/modern compiler stack-alignment contract.
- Context C returns the complete saved ESP; frame format lives in per-CPU context state. ESP low bits are not a frame-format transport in the current kctx17 backend. A process may use legal misaligned i386 ESP residues; kernel-entry code must preserve them. The frozen pointer-bit backend is historical, not the production ABI.
- Protected images use the existing a.out magic protection bit (bit 24) / protected-load flag and enter CPL3 through the established user-frame trampoline. Return/exit handling remains in proc.c and the SDK crt0/qd_exit path. Existing loader origin/profile, sizes, argument layout and error codes are retained; no ELF/relocation or new binary format is introduced.
- Job IDs are generation-tagged `(tag << 16) | slot`; validate the full ID, not only its low slot. Ownership of heap, channels, process records and child status uses the tagged ID.
- Scalar error returns are negative. Allocation APIs may encode errors in pointer-sized integer returns; wrappers must preserve signed error checks. Old exit-status/error-number ambiguities remain unchanged. A successful syscall need not imply a full requested read/write; callers must inspect counts.

## Pointer and lifetime rules

`pg_user_range_ok` accepts only mapped process image, argument page, committed private heap and private stack ranges, with NULL/overflow checks. Kernel pointers are not user buffers. Nonprocess legacy paths retain their flat range predicate. The kernel copies/validates argument structures and strings according to each trap's existing bounds; no arbitrary kernel callback supplied by a CPL3 process is supported.

The job has a shared supervisor kernel-entry stack, distinct from its private user stack. Page-table/frame operations run through the template CR3 on a kernel stack; a private CR3 change must never unmap the active kernel stack. Guard/fault handling terminates the protected job using the existing exception/double-fault paths. Kernel faults are not generally recoverable user errors.

File reads can suspend/restart at the trap boundary. A cache probe uses kernel temporary storage and commits cursor/user-buffer effects only after the whole probe succeeds. A killed reader's buffer is not retained by the worker. Callers of blocking operations must preserve valid owned buffers until the operation completes.

The diagnostic-only `QDOS_LEGACY_DIAGNOSTIC` bridge admits kernel-owned service buffers for non-application test clients. Loaded legacy application owners still use the strict predicate, and direct chan_user_range_ok stays strict. This adaptation is absent from production and is not a native userland ABI extension.

R3 preserves the SDK/user headers, trap constants, shared record sizes and all live service objects before conditional cleanup. Subsequent branch selection preserves semantic preprocessor tokens and the post-split images. ABI comparisons, map deltas and runtime fault/register evidence are recorded in the handover evidence. Legacy diagnostic success is not CPL3/SMP qualification.
