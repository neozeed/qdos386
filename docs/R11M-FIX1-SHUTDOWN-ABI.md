# R11M-FIX1: guest-requested clean shutdown of the Tiny386 full-machine emulator

Scope: one emulator (Tiny386 full machine; headless runner, windowed "rawdraw" frontend, SDL frontend) and one small guest helper
(`SHUTDOWN.AOU`). Nothing in the QDOS/386 kernel, the file system, paging, job control or the CPU core changed; the R11M kernels
and the frozen default images are the ones that were released.

## 1. Decision: reuse the existing port, add no new one

The plan suggested a new opt-in control port (for example `0xF4`). The decoded I/O space of the emulator was audited
(`pc_io_read/write`, 8/16/32 bit; list in section 7). Tiny386 **already** decodes one byte-wide shutdown port:

| Item | Value |
|---|---|
| Port | `0x8900`, byte `OUT` only (16/32-bit accesses are not decoded: an undecoded `outw` prints a diagnostic on stderr and is ignored) |
| Command | the 8-byte string `Shutdown` written one byte at a time (`'S' 'h' 'u' 't' 'd' 'o' 'w' 'n'`) |
| Convention | the Bochs BIOS "shutdown port" string. **Measured: QEMU 8.2.2 (`-machine pc`) does not act on it** (section 6) |
| Invalid writes | any byte that does not continue the string resets the matcher to the start; nothing else happens |
| Effect of the full string | `pc->shutdown_state` becomes 8. **No exit() is called from the I/O handler.** |
| Safe point | every frontend's main loop tests `pc->shutdown_state != 8` between `pc_step()` calls; the guest runs on for at most `PC_STEP_COUNT` instructions (512 / 4096 / 10240 by configuration) after the last byte |
| Reply | none (a write-only port); reading it returns 0xFF as every undecoded port |

Reusing it keeps the emulator's I/O decode unchanged and avoids a second shutdown mechanism. What the pre-FIX1 emulator lacked was not the request but the **teardown**: it simply
returned from `main()`, leaving the stdio flush of the disk images to `exit()`, unchecked and without a sync.

"Opt-in": the port is decoded in every configuration (it always was). A guest only reaches it by writing all eight bytes in order;
QDOS/386 does so only from `SHUTDOWN.AOU`, which a user runs deliberately. There is no ini switch and no change to any
default behaviour: a run that never writes the string behaves exactly as before.

## 2. Emulator lifecycle (`patches/tiny386-fix1-shutdown.patch`)

```
guest: OUT 0x8900,'S' ... 'n'        ->  pc->shutdown_state = 8              (device I/O: only sets the flag)
main loop (after the current pc_step) ->  loop ends
pc_shutdown(pc)                       ->  ide_flush_all(1)
    for every writable image (BF_MODE_RW, registered when it is opened):
        report an earlier failed fwrite              (flag set in bf_write_async)
        fflush(f)            error -> "fflush failed"
        ferror(f)            error -> "stream error"
        fsync(fileno(f))     (_commit on Windows)   error -> "fsync failed"
        fclose(f)            error -> "fclose failed"
    returns the number of images that failed
one line on stderr: "tiny386: guest shutdown request; disk images flushed and closed"   (or "... FLUSH FAILED")
process exit status: 0, or 3 if any image failed
```

* Read-only images (CD-ROM, `BF_MODE_RO`) and snapshot images are not registered and need no flush.
* The IDE model is synchronous (`bf_write_async` completes inside the I/O handler), so there is no pending IDE command when
  the loop has stopped. A guest that issues disk I/O after the last byte of the string and before the emulator stops (at most one
  `pc_step`) can still have that I/O performed; `SHUTDOWN.AOU` issues none.
* The same `pc_shutdown()` ends the headless runner (`main.c`), the windowed rawdraw frontend (both its window loop and its
  `-headless`/`-term` loop) and the SDL frontend (end of loop and `SDL_QUIT`). SIGTERM/SIGINT (headless and rawdraw) also leave the loop
  and go through it. `exit()` paths other than these (usage errors, a missing image at start, `abort()`) are unchanged.
* Portability policy: `fsync` on POSIX, `_commit` on Windows. No directory fsync (the image file exists before the run), no
  `F_FULLFSYNC` on macOS, no `O_DSYNC`. The call guarantees that the host has the data; it is not a promise about the drive's cache.

### Exit status

| Status | Meaning |
|---|---|
| 0 | stopped (guest request, SIGTERM, SIGINT, window closed); every writable image flushed, synced and closed |
| 3 | at least one writable image could not be flushed, synced or closed, or an earlier write to it had failed; the failure is named on stderr |
| 1 / other | unchanged: bad command line, ini error, an image that cannot be opened (`perror` + `exit(1)`) |
| killed by a signal | SIGKILL (or power loss): nothing runs; see the durability report |

## 3. Guest helper `SHUTDOWN.AOU` (`sdk/examples/r11m/shutdown/shutdown.c`)

```
RUN SHUTDOWN.AOU        check, then request the shutdown
RUN SHUTDOWN.AOU /N     check only: prints "SHUTDOWN: ready" or the refusal, writes nothing to the port
RUN SHUTDOWN.AOU /W     test only: writes "Shutdowm" (wrong last letter); a correct emulator ignores it
```

1. Lists the job table (`qd_job_list`). If any **launched** job other than the helper is alive it prints
   `SHUTDOWN: refused, N other job(s) still running (use PS and KILL)` and returns 1: such a job may be inside a file system operation.
   The shell itself is not a launched job and holds no open file between commands.
2. Prints `SHUTDOWN: requesting`, flushes the C library streams. The QDOS/386 file system is write-through (data, FAT 1, FAT 2,
   directory entry are on the emulator's disk interface when each call returns), so nothing else needs to be written first.
3. Writes the eight bytes with `outb` (all code runs in ring 0, there is no system call for port I/O; the helper is the only code that
   does it).
4. Waits up to about two seconds (200 × `qd_yield(1)`). On an emulator that honours the request the machine is gone before this
   ends. If it is still running, it prints `SHUTDOWN: no response` and returns 2. That is the correct result on an emulator or board
   without the port, for example the CPU-only i386VM harness, and for the wrong-magic test.

It is **not** an OS shutdown service: it does not stop jobs, does not unmount, does not park the disk, and on real hardware it does nothing.
ATA FLUSH CACHE (0xE7) is not issued: Tiny386 does not implement it and the kernel's PC disk driver was left unchanged.

## 4. Errors and what is reported

| Situation | Behaviour |
|---|---|
| wrong or partial string | ignored, matcher reset; the helper reports `no response` (status 2) and the machine keeps running |
| other jobs running | helper refuses (status 1); nothing is written to the port |
| no writable image was ever written | exit 0; image byte-identical to the start |
| `fflush` / `ferror` / `fsync` / `fclose` fails | named on stderr, `disk images FLUSH FAILED`, exit status 3 |
| an earlier `fwrite` to the image was short | reported at shutdown, exit status 3 (the guest is **not** told: the IDE protocol is unchanged) |
| shutdown requested twice | the second string cannot arrive: the machine has stopped |

## 5. What this does not do

* It does not make writes crash-safe. SIGKILL, a host crash or power loss before the request can still lose the last buffered
  sector write (the original finding; reproduced again in the durability report). A graceful, guest-requested exit is the supported path.
* It does not make the guest wait for the host: the emulator's disk writes are still buffered by stdio between the guest's
  writes; the flush happens at shutdown (or at the next seek on that image, as before).
* It does not change FAT16/VFAT, the ATA register protocol or any guest-visible behaviour except that the machine can be stopped.

## 6. Frontends and other emulators

| Environment | State |
|---|---|
| Tiny386 headless runner (`tiny386_headless`) | built and tested (matrix in `docs/R11M-FIX1-RUNTIME-REPORT.md`) |
| Tiny386 windowed rawdraw frontend (`tiny386`) | built; tested under Xvfb (see the runtime report for the exact scope) |
| Tiny386 SDL frontend (`sdl/main.c`) | patched identically but **not compiled here** (no SDL development files in the build environment) |
| QEMU 8.2.2 `-machine pc` | **does not honour the string** (tested: the helper prints `SHUTDOWN: no response`, status 2, QEMU keeps running). QEMU runs are ended by the harness through the monitor (`quit`); QEMU's raw image backend writes straight to the host file (no stdio buffer) and every QEMU round in R11M and FIX1 left clean images. The required QEMU path uses no Tiny386-specific port |
| CPU-only i386VM harness | no such port; the helper reports `no response`; unchanged and retired from gating |

## 7. I/O decode audit (Tiny386 `pc.c`, before the patch)

Byte reads: 0x00-0x0F, 0x20-0x21, 0x40-0x43, 0x60-0x61, 0x64, 0x70-0x71, 0x81-0x83, 0x87, 0x89-0x8B, 0x8F, 0x92, 0xA0-0xA1, 0xC0-0xDE (even),
0x170-0x177, 0x1F0-0x1F7, 0x220-0x22F, 0x2E8-0x2EF, 0x2F8-0x310, 0x31F, 0x376, 0x388-0x38B, 0x3B4-0x3BA, 0x3C0-0x3DF, 0x3E8-0x3EF, 0x3F6, 0x3F8-0x3FF,
0x481-0x48F, 0xCFC-0xCFF, 0xF1F4. Byte writes add 0x80, 0xED, 0x402 (debug character port) and **0x8900**. `0xF4` is not decoded either way.
The patch adds no port. Patched files: `ide.c`, `ide.h`, `pc.c`, `pc.h`, `main.c`, `rawdraw/main.c`, `sdl/main.c` (185 diff lines).
