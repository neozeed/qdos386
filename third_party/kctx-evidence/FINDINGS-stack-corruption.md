# GW-BASIC on QDOS/386 R13B: long FOR/NEXT and GOSUB double fault

Status as of 2026-10-30. Diagnosis complete. No fix applied. The kernel change and the port change are both
unapplied and both need a decision (see section 7).

## 1. Summary

Long FOR/NEXT loops and a long GOSUB crash the R13B VGA image with `#DF double fault, eip 00000024`. The
root cause is a stack-alignment mismatch between the port and the kernel:

- A FOR entry is 26 bytes (`FE_SIZE`, `src/forent.inc`). Its base is 2 mod 4, so while a FOR entry is on the stack,
  the program's ESP is 2 mod 4. This holds through the loop body and the path back to the NEXT chop.
- The kernel assumes ESP is 4-byte aligned. When it preempts a job, it saves `saved_sp | fmt` and restores it with
  `andl $-4`, which discards bit 1. The restored frame is then read 2 bytes low. `iret` loads EIP=0x24 and a wrong
  ESP, and the CPU double-faults.

The failure depends on how many iterations run before a timer tick lands inside a FOR body, so it is probabilistic.
That is why the threshold looks like about 8100 to 16000 iterations and is not exact.

I confirmed the mechanism with a BASIC-independent control. A small assembly program with ESP 2 mod 4 double-faults
when the kernel preempts it. The same program with ESP 0 mod 4 exits 0.

## 2. Build identity (preserved, not modified)

Failing checkpoint, packaged as `failing-checkpoint.zip`:

| File | SHA-256 |
|---|---|
| `GWBASIC.AOU` | `4ae47943c9015086c49fcb7e1ecb385ce1fe0fb28a16ca5bc103f8ca023f461d` |
| `launch-volume.img` | `ed03cd27238bdfb84e4b39e2e11850acd1f7d805fd05d7c7d66df6512a7926d7` |
| `r13b-vga.img` (kernel image) | `0199a72b715db21f7d1bf9d2c2cdd8ea4742b59d67bb06d40e9012d8448d8f74` |

Kernel source (read-only, from the R13B archive, unmodified):

| File | SHA-256 |
|---|---|
| `kernel/kctx.s` | `5714e2db86be4d1041de8ac619026bc7263bc3dda42719c220956d5e4d29655b` |
| `kernel/jobs.c` | `7847d9b0b6c2dddb34310b3d3dfdb0b22a7ab63afcc7cfa1e8e7f925b39505c1` |
| `kernel/jobs.h` | `6ac6ec939aa39d56e6343bd9c47019816418418cd92ba41a0ba8cbf15eaf70e3` |
| `kernel/context.s` | `4a4e05a18290d3564dd82d98535e9a3ea90a5c343858a96c9447039ac22bd3f9` |

QEMU command line (from `qemu-cmdline.txt`):

```
qemu-system-i386 -machine pc -cpu pentium -m 16 -bios /usr/share/seabios/bios.bin -vga std -display none \
  -monitor unix:<scratch>/mon.sock,server,nowait -serial file:<scratch>/serial.log \
  -drive file=<R13B>/build/r13b/pc-vga/r13b-vga.img,format=raw,if=ide,index=0,media=disk \
  -drive file=<scratch>/fail-ckpt/launch-volume.img,format=raw,if=ide,index=1,media=disk -boot order=c
```

Trace runs add `-d int -D <log>`. The harness scripts are in `harness/` (they use the scratch layout and need the paths edited to run elsewhere). The BASIC runs type `gwbasic`, wait, then type the program and `RUN`.

## 3. Reproducers

In the repo (`qdos/repro/`):

- `spin_misaligned.S`: ESP 2 mod 4 for the whole spin. Expected: double fault on preemption.
- `spin_aligned.S`: control, ESP 0 mod 4. Expected: exit status 0.
- `basic/*.bas`: the BASIC programs, with the observed results in `basic/README.txt`.

Results on the R13B VGA image:

| Test | Result |
|---|---|
| flat FOR/NEXT, 1100 / 2100 / 4100 / 8100 iterations | pass |
| flat FOR/NEXT, 16000 / 16400 / 20000 / 200000 iterations | `#DF`, eip 00000024 |
| nested FOR/NEXT, 100 inner × 1000 outer | pass |
| GOSUB, 20000 calls | `#DF`, eip 00000024 |
| `spin_misaligned` (ESP 2 mod 4, no BASIC) | `#DF` |
| `spin_aligned` (ESP 0 mod 4, no BASIC) | exit 0 |
| SAVE/LOAD | works on short programs; not yet run on a long program |
| String operations | not verified. The first test used a numeric `A` and failed with `Type mismatch`, which was a test error. A corrected `A$` test has not been rerun. |

Win32 build: 23/23 at the last run (before this session's work). This session changed no `src/` file, only
`NOTES.md` and the new `qdos/repro/` directory.

## 4. Evidence

The fault, from the QEMU interrupt log (`int-i16000.log`, entry near line 727909):

```
v=08 e=0000 i=0 cpl=3 IP=0024:00000024 pc=00000264 SP=0024:847a0000
EAX=00d50000 EBX=00000025 ECX=00000024 EDX=04000024
ESI=5c000025 EDI=00d5980a EBP=b9b20025 ESP=847a0000
EIP=00000024 EFL=00060002 CPL=3
CR0=80000011
```

The job record for job 0x10003, read from a RAM dump taken at the fault (`ram-d16000.bin`):

- `saved_sp = 0x0025b922` (2 mod 4)
- `saved_fmt = 1` (interrupt frame)

The app's timer tick was taken at SP `0x0025b94e` (also 2 mod 4). The kernel saved that frame unchanged, then
`resume_dispatch` masked bit 1 on the way back.

CR0 is `0x80000011` (PG, ET, PE). Bit 18 (AM, alignment check) is clear, so a misaligned stack access does not raise
#AC. The CPU therefore accepts the misaligned frame, and the failure is entirely in the kernel's masking.

## 5. The mechanism, with source locations

Port side:

- `src/gwfor.S` line 120: `sub esp, FE_SIZE` reserves the 26-byte FOR entry, so B = ESP is 2 mod 4 from that point.
- `src/gwfor.S` line 121: `FORFRMB` stores B, which is the entry base used by FNDFOR.
- `src/next86.S` lines 39 and 114: `mov esp, ebx` chops the stack back to B and then to the updated stack.
- Other stack changes in the port are even: `add esp, 2`, `add esp, 4`, `add esp, 6`, `sub esp, 8`, and
  `sub esp, FE_SIZE` (26). So ESP stays even as long as the `mov esp, ebx` paths carry even values. I checked the
  adjustments by grep. I did not prove every `mov esp, ebx` source value, though nothing I found produces an odd one.

Kernel side, for the R13B image (`PCCTX = kctx.o`, Makefile line 199 and the pc-vga image rule at line 395):

- `kernel/kctx.s` lines 138–140: the IRQ0 stub. It pushes `pushal`, then `pushl %esp`, then calls
  `sched_irq_tick`, then jumps to `resume_dispatch`.
- `kernel/kctx.s` lines 43–52: `resume_dispatch`:

  ```
  resume_dispatch:
  	movl %eax,%ecx
  	andl $1,%ecx          /* fmt, from bit 0 */
  	andl $-4,%eax         /* line 46: discards bit 1 of a misaligned ESP */
  	movl %eax,%esp
  	cmpl $0,%ecx
  	je resume_ctx
  	popal
  	iret
  resume_ctx:
  	popl %edi
  	popl %esi
  	popl %ebx
  	popl %ebp
  	sti
  	ret
  ```

- `kernel/jobs.c` lines 261–311: `schedule()` stores `out->saved_sp = sp` and `out->saved_fmt = fmt`, then returns
  `saved_sp | saved_fmt`. The comment in `context.s` lines 9–10 says "Stacks are 4-byte aligned, so bits 0-1 are free".
  That comment is the kernel's stated contract.
- `kernel/jobs.c` lines 314–322 (`sched_irq_tick`) and 383–388 (`sched_yield_sp`) are the two entry points.
- `kernel/jobs.c` lines 107–120 (`init_frame`) build the initial frame at `stack_top - 20`. `stack_top` is a 16-byte-aligned heap block (`kernel/jobs.h` line 29), so the initial frame is aligned.

The same `andl $-4` appears in two other files:

- `kernel/context.s` line 48: used by non-DYNPROC images. Not linked into R13B.
- `kernel/paging.s` line 48 (`pg_exc_common`): used only by the paging targets (Makefile lines 280–330). Not linked
  into R13B, since `R13B_OBJS` does not include `paging_s.o`.

Any kernel change must update the file the target actually links. For R13B, that is `kctx.s` line 46.

## 6. Withdrawn hypotheses

- The 16-bit BX and DX operations in `FADDS` and `FCOMP` do not change ESP. Their push and pop pairs in `_FADDS`,
  `_FCOMP` and `_ROUNS` balance.
- The ESP range checks inside NEXT, FNDFOR, NXTCON, NEWSTT, `_FADDS` and `_FCOMP` never fired.
- The width audit found five PTR accesses wider than declared (FACLO/_FACM1, DIMFLG/VALTYP, FSTCOL/LSTLIN). All are
  intentional overlaps in the FAC block or adjacent flag bytes. None touches a stack-restore variable.
- The earlier note "eip 0x240024" was wrong. The report shows eip `00000024`. Corrected in NOTES.md.

## 7. Kernel change options

Both options need your decision. The question is whether the kernel should accept a 2-mod-4 saved ESP, or whether
the port should stop producing one. The kernel's own comment (context.s lines 9–10) says stacks are 4-byte aligned.
So the port breaks a documented contract, and the kernel's resume code fails silently when the contract is broken
rather than faulting. Both views are defensible, which is why this is your call.

### Option A: one-line change in `kctx.s` (recommended if the kernel may change)

Change `kctx.s` line 46 from `andl $-4,%eax` to `andl $-2,%eax`. The patch is in
`kernel-proposals/kctx-option-A.patch`, and I checked that it applies cleanly against the unmodified `kctx.s`.

Why it works:

- `andl $-2` keeps the format bit and discards only bit 0. Bit 0 has already been copied into ECX on line 45.
  The restored ESP keeps bit 1, so the frame is read from the correct address.
- `pushal`, `popal`, `pushl`, `popl` and `iret` have no alignment requirement when AM is clear (see section 4).
- The `return sp | 1u` sites in `jobs.c` (lines 637, 643, 655, 1036) are unchanged and still correct, because
  `(sp | 1) & ~1` is `sp` when sp is 2 mod 4.
- The kernel entry paths (`kctx.s` lines 60–110, the `kx_entry` switch) compare ESP against a range and do 4-byte
  pops. None of them assume alignment.
- Stack canaries sit at `stack_base`, not at ESP, so `stack_faults` and `stack_hwm` are unaffected.

Precondition: saved_sp must be even, so that bit 0 is free for the format flag. The port satisfies this (section 5).
If it were ever violated, bit 0 would be read as the format flag and the wrong restore path would run. I did not add
a guard. A debug-only check in `schedule()` (`if (sp & 1) stack_faults++`) would catch a violation if you want one.

Cost: it widens the documented encoding, so context.s lines 9–10 and the `kctx.s` header comment must be updated.
`context.s` and `paging.s` should get the same one-line change for consistency, though neither is linked into R13B.

### Option B: no change to the ESP value at all

Store the format flag in a global, `resume_fmt`, set in `schedule()` (`jobs.c` line 268 and each return). Then
`resume_dispatch` reads `movl _resume_fmt,%ecx` instead of `andl $1,%ecx`, and the ESP is restored without any mask.
The four raw `return sp | 1u` sites (`jobs.c` 637, 643, 655, 1036) must also set `resume_fmt = 1`.

This removes the even-ESP precondition, but it touches more of `jobs.c` and changes the return convention for every
scheduler entry. It is the more robust fix. It is also the less minimal one.

### Port-side fix (not recommended as the first step)

Keep ESP 4-byte aligned everywhere the kernel can preempt. This means a 28-byte FOR entry with the offsets in
`forent.inc` updated, and converting the 2-byte pushes and pops to 4-byte ones, with FNDFOR, NEXT, GOSUB/RETURN and
WHILE/WEND updated to match. It is large (NOTES.md estimates several hundred 16-bit register saves), and Win32 can
only check layout consistency, not the kernel interaction.

### Test plan if Option A or B is approved

1. Apply the change in a copy of the R13B tree. Do not modify the archive. Force a rebuild of
   `build/r13b/pc-vga/r13b-vga.img`, since the normal make target did not rebuild automatically last time.
2. Run `spin_misaligned` (expect exit 0) and `spin_aligned` (expect exit 0).
3. Run flat 16000, 16400, 20000 and 200000; nested 100×1000; GOSUB 20000; and SAVE/LOAD on a long program.
4. Run the Win32 `make test` (expect 23/23).
5. Check whether the R13B tree has its own kernel tests. I have not checked this yet.

Limitation: every result so far comes from QEMU's `pentium` model. I have not run this on real hardware.

## 8. What the repo contains after this session

- `NOTES.md`: Stage 5 root-cause section, with the reproducer pointers and the corrected kernel line references
  (`kctx.s` line 46, not `context.s` line 48).
- `qdos/repro/spin_misaligned.S`, `spin_aligned.S`, and `basic/` with the BASIC programs and `README.txt`.
- No changes to `src/`, `qdos/*.S`, `Makefile*`, or the R13B kernel.

## 9. Other open items

- FNDWND (`src/fiveo.S`, around lines 77–95) skips a FOR entry with `FORSZC-1` = 25. It should skip
  `FE_SIZE-2` = 24. This is latent: it only matters for WEND with a FOR still on the stack. Not changed. It needs a
  WHILE/WEND test first.
- WHLSIZ is defined twice: `forent.inc` sets 10 and `fiveo.S` sets 11. Both give the same WHILE skip of 10 at their
  one consumer, so the values agree in effect. The name means different things in the two files. Not renamed.
- The string-operations reproducer needs its corrected `A$` test rerun.
- File I/O with `#` file numbers (`FILGET`, `stage3stubs.S`) is still deferred to ADVERR. Graphics and the other
  unsupported statements are unchanged.
- SuperBASIC (`BASIC.AOU` in the R13B archive) is not redistributable, and is not part of any artifact from this build.
- The GAS 1.38 requirement and the 64 KB limit are unchanged. The port still makes no claim of full GW-BASIC
  compatibility.

## 10. Decision needed

Choose one:

1. Option A (one-line kernel change, recommended if the kernel may change). Smallest change, no port change.
2. Option B (kernel change without altering ESP). More robust, more invasive in `jobs.c`.
3. Port-side 4-byte alignment. No kernel change. Large.

Option A's precondition (even saved ESP) holds for everything I checked. Option B has no precondition.
