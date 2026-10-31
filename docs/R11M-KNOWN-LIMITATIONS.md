# R11M known limitations

What R11M is: an OS-facilities milestone on top of the frozen R11L base (FAT16 subdirectories and native paths, four virtual text consoles, job control, VFAT long
names). What it is not: memory protection, a native compiler rebuild, a GUI. Nothing here claims isolation between programs.

## 1. Isolation and memory

* **No protection.** All code runs in ring 0 in one address space. Paging (R11K) separates only the *image windows* of paged programs; heaps and stacks are shared.
  A program can overwrite the kernel or another program. `KILL` does not check who calls it and a launched job can be killed by any other.
* A **paged job cannot start another job** (and cannot kill). Only the shell (slot 0) launches. The shell's job record holds 8 jobs.
* `JOB_MAX` is 6 (system, shell, idle, three more); `PG_PROC_MAX` 3; kernel stacks are 4 KiB per job. Starting more than fits is refused with a bounded code.
* Kernels without `-DQDOS_PAGING` run one `START`ed program at a time (the slot 1 loader). The frozen default images contain none of the R11M code and are byte-identical to
  R11L (`scripts/r11l-default-check.sh`, six hashes).

## 2. Consoles

* Four consoles, 80x25 text, no scrollback, no escape-sequence terminal, one input waiter per console, the diagnostic stream (serial/debug port) is shared.
* Two jobs on the same console share its input queue and cursor.
* PC: Alt+F1..F4 or Alt+1..4 select the console (the key is consumed by the kernel). The i386VM harness has no display or keyboard: focus is switched by ESC+digit bytes
  in the input stream and the screen is read back with `CONDUMP`. The harness also drops byte 0x7F (DEL), so backspace editing is tested with 0x08 there; the kernel
  accepts both.
* On i386VM the harness input is already at end-of-input when the sequence finishes, so the 4 JOBT checks that block a job on console input are **NA** there (65/65 of
  the checks that can run; 69/69 on Tiny386 and QEMU).

## 3. File system

* FAT16 only; no FAT32, no partition table, no GRUB. One volume (`WIN1`). Path ≤ 64 characters, 16 levels, component ≤ 63 (VFAT) or 8.3.
* No crash atomicity: writes are ordered (data, FAT 1, FAT 2, directory entry) and write-through, but a power cut between steps can leave orphan clusters, a duplicate name
  during a rename, or a short entry whose long-name chain was not written. `tools/r11m-fatwalk.py` reports these; nothing repairs them.
* A directory cannot move to another parent. Directory entries have no size or timestamps beyond what the FAT tools set. The root directory has a fixed size.
* VFAT: printable ASCII 1..63 characters only; no Unicode, no case-preserving short names; an entry whose 8.3 alias contains a byte above 0x7E is skipped. Details,
  alias algorithm and trust rules in `docs/R11M-VFAT-LIMITATIONS.md`.
* The legacy `WIN1_NAME` interface is unchanged and flat (upper-case 8.3, root only); underscores are never interpreted as directory separators.
* `COPY` copies files, not directories; with `/Y` a failed copy can leave a truncated destination.

## 4. Shell and programs

* `WAIT` blocks the shell; there is no Ctrl-C. A runaway foreground `RUN` can only be ended by the user from outside (reset) unless it is a `START`ed job (`KILL`).
* Words with blanks need double quotes (`COPY A "Long Name.txt"`); no wildcards, pipes or redirection; quotes cannot be nested or escaped.
* The Phoon and SDK programs are ordinary slot-1 a.out images; they are not relinked per directory. Programs linked for another origin cannot be `START`ed (-61).
* Zork I was not run: no private story file was provided and none was invented or bundled.
* The native GCC toolchain of R11L is untouched and was **not** run as a test here (it is not part of this milestone).

## 5. Emulators and the test method

* **Release verification policy (changed during finalization).** QEMU VGA is the primary integration and regression environment. The standalone CPU-only i386VM
  harness and the interpretive Tiny386 *headless* runner are legacy diagnostic targets, not release gates; their existing passes are kept and labelled with the
  exact configuration. The windowed full-machine Tiny386 build was **NOT TESTED**. A third Tiny386-headless matrix round was stopped by decision after
  its first three cases (`logs/r11m/pc-r3.*`, not counted).
* **Open storage finding (NOT fixed here; no crash-safe-write claim).** One Tiny386-headless run left an image whose two FAT copies differ.
  * Sequence: kernel `build/r11m/pc-pg/r11m-pg.img`, volume `scripts/r11m-demovol.sh`, `tests/r11m/seq-s3a.txt` (`PS`, `RUN JOBT.AOU`, `PS`), run by `scripts/r11l-run-pc.sh`
    (the harness ends the emulator with SIGKILL right after the last prompt). Reproduced twice (the original matrix run and a dedicated rerun); the image is `logs/r11m/noflush/tiny386-nf.final.img`
    with `fsck.txt` / `walk.txt`.
  * Result: `fsck.fat`: "FATs differ but appear to be intact"; `r11m-fatwalk.py`: "FAT copies differ in 1 entries". The entry is cluster 3168: **FAT 1 = 0x0000 (free), FAT 2 = 0xFFFF (end of chain)**.
    JOBT's last action is removing the file its killed `WRITER` job had been writing, so this is the freeing of that chain: copy 1 was written, copy 2 was not seen on disk.
  * What is established: the same sequence is clean on QEMU (every round) and on i386VM; the same Tiny386 run is clean if one more disk *read* follows; Tiny386's IDE model writes the image with
    `fwrite` on a stdio `FILE` and never calls `fflush`/`fsync` (`ide.c`, `bf_write_async`), and does not implement ATA FLUSH CACHE (0xE7 is only a `#define`). The kernel's FAT writer
    writes FAT 1 then FAT 2 synchronously (R11L ordering).
  * What is **not** established: that the guest kernel issued the FAT 2 write before the harness killed the emulator was not separately traced (no guest-side instrumentation was added). The
    explanation "lost in the emulator's stdio buffer" is the best fit, not a proof.
  * Workaround, test-harness only: `scripts/r11m-run-pc.sh` appends a harmless read before the final prompt so the Tiny386 runs reach the image file. **This is not a durability guarantee** and
    must not be read as one: a real power cut or kill at any point can leave the two FAT copies different, and nothing in R11M detects or repairs that at boot.
  * Why no kernel fix now: a guest cannot flush the host's stdio buffer; issuing ATA FLUSH CACHE after FAT updates would be correct for real disks and QEMU but is a no-op/aborted command on this emulator, and
    changing the PC disk driver late in the milestone is out of scope. Deferred to a dedicated storage-hardening milestone (explicit flush/barrier after the FAT and directory updates, a
    mount-time FAT-copy comparison, fault-injection tests).
* **QEMU** is driven through the monitor (`sendkey`, `xp /4000xb 0xb8000`): transcripts are sequences of VGA snapshots, so a line can be caught mid-update and identical lines
  cannot be counted; checks use distinct content. Wall time does not track guest time (screen sampling is slow), so background jobs are given long counts and the
  progress comparisons use strictly increasing values rather than rates.
* i386VM, Tiny386 and QEMU share one test volume layout but have different timing; sequences use explicit completion markers and bounded timeouts instead of fixed sleeps.
* Hardware (a real PC) was not tested.

## 6. Test coverage that is stated, not hidden

* Not tested: FAT32; non-ASCII names; guest runs on volumes larger than 3 MB (the host tests also use 10 and 20 MB shapes); disk errors during a directory update; kill of a job in the middle of a FAT update (traps are atomic with
  interrupts off, not separately injected); more than 3 paged jobs at once on a PC map with both frame windows in use (only the exhaustion section of JOBT).
* The independent comparisons are `fsck.fat` 4.2, mtools and `tools/r11m-fatwalk.py` (written for this milestone; it shares no code with the kernel but has the same author).
