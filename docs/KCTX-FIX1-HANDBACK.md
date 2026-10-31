# KCTX-FIX1 hand-back note — for the GW-BASIC builder

**Result:** the reported fault is a real kernel defect, present unchanged from R12A to R14/R14A (`kernel/kctx.s`
`resume_dispatch`, `andl $-4`). It is fixed in a separate corrective kernel, **KFIX1**; the frozen R13B/R14 releases
are untouched and still contain the defect. Your `GWBASIC.AOU` was not changed.

## Use this kernel

| File | SHA-256 |
| --- | --- |
| `kernels/kfix1-vga.img` (QEMU) | `f8e5d5e1f7e5c698ed13cc57159f7c9a7faeffb8fcf523e185625a57aec0d4c3` |
| `kernels/kfix1.img` (headless / Tiny386) | `7c67597862f24de2ef2237d32540976b4285ed20f7a4bfe1dd6508248af395b0` |
| your `GWBASIC.AOU` (tested unchanged) | `4ae47943c9015086c49fcb7e1ecb385ce1fe0fb28a16ca5bc103f8ca023f461d` |
| your `launch-volume.img` (tested on a copy) | `ed03cd27238bdfb84e4b39e2e11850acd1f7d805fd05d7c7d66df6512a7926d7` |

```
qemu-system-i386 -machine pc -cpu pentium -m 16 -bios /usr/share/seabios/bios.bin -vga std -display none \
  -monitor unix:<scratch>/mon.sock,server,nowait -serial file:<scratch>/serial.log \
  -drive file=<pkg>/kernels/kfix1-vga.img,format=raw,if=ide,index=0,media=disk \
  -drive file=<copy of launch-volume.img>,format=raw,if=ide,index=1,media=disk -boot order=c
```

The kernel is the R14 kernel (R13B + serial) with the corrected resume path; your volume's R13B shell and programs
run on it unchanged.

## What was wrong

Every kernel entry pushes the interrupted job's registers on the job's own stack; the C handler returned
`saved_sp | fmt` and `resume_dispatch` rebuilt the stack pointer with `andl $-4`. With ESP ≢ 0 mod 4 (legal: CR0.AM is
clear and no QDOS/386 ABI requires alignment) the job was resumed 1–3 bytes below its frame. This happens on the
**first** kernel entry with such an ESP — a timer tick, a service trap (any I/O call) or a yield — not only on
preemption. gdb trace of the first divergence: `logs/kctx/stage1/trace.gdb.txt` (frame saved at `…f0e`, decoded to
`…f0c`, `iret` loads CS 0x0024 — your `IP=0024:` report).

**Fix:** the handlers return the full stack pointer; the frame format goes in a separate kernel variable written just
before the return (interrupts off throughout). All residues 0–3 resume exactly. The “Option A” mask change was not
used: it would still mis-resume odd ESP values.

## Tested (logs/kctx/)

| Test | R14 kernel (before) | KFIX1 |
| --- | --- | --- |
| your `spin_aligned` / `spin_misaligned` (assembled unchanged) | exit 0 / **#DF, halt** | exit 0 / exit 0 |
| ALIGNT residues 0,1,2,3 × spin (thousands of preemptions), 200 000 traps, 300 yields; two concurrent misaligned jobs | residue 0 only; 1, 2 #DF, 3 #GP | 22/22 |
| your GWBASIC.AOU: flat FOR/NEXT 16000, 16400, 20000, 200000; nested 100×1000; GOSUB 20000 | halts at 16000 | **all DONE / Ok** |
| Tiny386 (full machine) probes + Hack smoke | — | see `docs/KCTX-FIX1-REPORT.md` |

## Still open — interpreter side (not kernel; GW-BASIC unchanged)

1. **SAVE fails** with `Device I/O Error` for every name tried (`"SHORT"`, `"SHORT.BAS"`, `"/HOME/S2.BAS"`), started
   from `/` or `/HOME`, foreground or `START … /NEW` — **identically on your checkpoint's own R13B kernel**, on R14
   and on KFIX1 (`logs/kctx/stage3/saveload-ab/`). LOAD then says `File not found`. `FILES` says `Advanced Feature`.
2. **String test:** `A$="HELLO"`, `B$=A$+" WORLD"`, `PRINT B$;LEN(B$)` prints `HELLO WORLD 11` (correct). Adding
   `PRINT MID$(B$,7,5);LEFT$(B$,5);RIGHT$(B$,5)` makes GW-BASIC take a page fault in its own code: `#PF eip 00241495
   cr2 b9f80024`, EBX `b9f80024`, EBP `0024b9fd` (halves of a 32-bit value read 2 bytes off — a port-side 16/32-bit
   access). The kernel ends only GW-BASIC (“process ended”) and the shell continues. On R13B/R14 the run never gets
   this far (the kernel defect halts first), so this fault was hidden until now.
3. `SYSTEM` answers `Advanced Feature` (cannot leave GW-BASIC except by Task Manager/`KILL`).

Not claimed: that GW-BASIC is correct. Only that its loop/GOSUB double fault was the kernel's and is gone.
