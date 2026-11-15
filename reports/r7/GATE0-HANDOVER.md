# Apollo R7: baseline verified; Git checkpoint blocked

Resume update, 2026-11-15: Git writes are now available. All 2319 initial
filesystem entries were rechecked unchanged without repeating passing builds
or guest tests. Created `r7-safety-da9d217` at the baseline HEAD and continued
on `r7-native-disk-tools`. The blockage described below is historical; Stage A
implementation is the next gate. The baseline evidence remains applicable.

2026-11-15 UTC. Current authoritative checkout: `/data/projects/qdos386`.
Branch `main`, HEAD `da9d217dacbfa2d1c02590aae87d1a95ca15678f`.
The only initial working-tree change was the untracked R7 mission document.
Remote name: `origin`. No network operation was attempted.

## Status and exact resume point

Gate 0 build/runtime/preservation verification passed. Its Git checkpoint is
blocked. Stage A, Stage B and Gate C are **not implemented or accepted**.
No R7 release, acceptance tag or source commit has been created.

The authorized safety-branch command `git branch r7-safety-da9d217` failed:

```text
fatal: cannot lock ref 'refs/heads/r7-safety-da9d217': Unable to create '/data/projects/qdos386/.git/refs/heads/r7-safety-da9d217.lock': Read-only file system
```

This execution environment permits source writes but mounts `.git` read-only.
User authorization is already sufficient; the filesystem restriction prevents
the required small commits and freeze checkpoints. No escalation, alternate
Git directory or history rewrite was attempted. A complete filesystem safety
checkpoint exists under `/tmp` (below). Kernel and application source changes
have been deferred until the required Git checkpoint workflow is available.

Resume with Git metadata writable: inspect HEAD/status and the saved evidence,
create the safety/development branches without disturbing the user's work,
commit the Gate 0 source report, then begin Stage A1. Do not repeat the passing
baseline builds or guest smoke without changed inputs or another causal reason.

## Preserved evidence and build commands

Workspace: `/tmp/qdos386-r7-1wn7185l/`.

- `checkpoint/source/`: actual initial filesystem copy, preserving modes,
  symlinks and untracked inputs; administrative/credential directories omitted.
- `checkpoint/BASELINE.json`, `TRACKED-SHA256.json`, `git-ls-files.txt` and
  `.gitignore`: initial Git refs/status and source hashes/modes.
- `worktree/`: isolated build copy; all in-tree Makefile products are here.
- `hosttools/`: copied external toolchain from the explicitly inventoried
  `/data/projects/qdos386.old/hosttools`, not an old source-tree substitution.
  `TOOLCHAIN-SHA256.json` records its inputs. GCC 1.40, GAS 1.38, i386-ld
  1.94, the converter and compiler runtime sources remain external dependencies.
- `BUILD-ATTEMPTS.json`, `ORACLE-COMPARISONS.json`, `logs/`: commands/results.
- `GATE0-DISK-VERIFICATION.json`, `GRAPHICS-RESULT.json`,
  `ORIGINAL-PRESERVATION.json`: independent post-run checks.
- `guest.py`, `check-graphics.py`, `run-gate0-build.py`,
  `check-baseline.py`, `finalize-gate0.py`: retained temporary test harnesses.

From the isolated `worktree/`, with `TMPDIR` and `PYTHONPYCACHEPREFIX` set to
the workspace's `tmp/` and `pycache/`, these commands passed sequentially:

```sh
make -B HOSTTOOLS=/tmp/qdos386-r7-1wn7185l/hosttools
make -B current-shell HOSTTOOLS=/tmp/qdos386-r7-1wn7185l/hosttools
make -B production64 HOSTTOOLS=/tmp/qdos386-r7-1wn7185l/hosttools
make -B apollo-r6 HOSTTOOLS=/tmp/qdos386-r7-1wn7185l/hosttools
python3 tools/apollo/build-boot.py /tmp/qdos386-r7-1wn7185l/apollo-boot
```

All **19** separate R5 24-job, R5 64-job, R6A and BIOS comparisons match the
previously verified oracles. The root Makefile delegates to `source/Makefile`
and `source/mk/current.mk`. SDK/apps/platform/port/fonts authority remains under
`source/`; no layout changes were made. SDK libraries and historical application
payloads were not rebuilt in this gate; the existing accepted disk supplied its
native compiler and applications, explicitly identified below.

## FAT16 fallback and actual guest checks

Accepted input: `/tmp/qdos386-root-build-1k76wt6b/qdos386-r6a.img`.
Independent safety copy: `checkpoint/fat16-fallback.img` in this R7 workspace.
Both retain SHA-256:

```text
3b3ceb41210c9726af2cd65c55225a5571f9df7193c96d722ce017506560396a
```

34,603,008 bytes; one active MBR primary FAT16 partition, type 0x0e,
start LBA 2048, length 65536 sectors, 512-byte sectors. Fresh R6A package hash:
`447b29fe0e08d052961cdff33e44233cec8ab9a1c8e94d6a40158fe12b658c1d`.
Fresh matching kernel/loader hashes establish equivalence to the accepted disk
components; this gate did not claim a fresh rebuild of its populated volume.

QEMU used exactly one ordinary disposable disk, TCG, Pentium, four CPUs,
32 MiB, VGA, no network and QMP over stdio. Full launch commands are in
`logs/gate0/command.json` and `logs/gate0-graphics/command.json`.

- Validated boot disk identity and all four CPUs online; native shell reached.
- `PS`: `IOW0` present; `RUN R20T`: 40 functional requests, 300 race pairs,
  stale-ticket and queue-full checks, `R20T FUNC PASS`, exit 0.
- `RUN GCC -o /TMP/R7HI.AOU /HOME/HI.C`: exit 0; executing the newly generated
  program printed `FIX1 guest GCC works!`, exit 0.
- Bounce first completed 120 frames, zero errors and matching model checksum.
  Task Manager displayed its live graphics session. That run had zero visible
  frames, so a focused follow-up tested actual presentation switching.
- Follow-up: Ctrl+Esc, End, Enter selected Bounce; Alt+F1 returned to the shell.
  180 frames, 45 shown, 135 hidden, zero errors, checksum/model 618968530,
  `verify=PASS`, exit 0. Screenshots and persisted `BOUNCE.LOG` retained.
- The follow-up's initial host screen judge rejected an 80-column word wrap
  (`verif` / `y=PASS`). The complete debug line and persisted file independently
  establish PASS; the temporary judge was corrected without rerunning QEMU.
- Both resulting partitions pass `fsck.fat -n` and the independent strict VFAT
  walker. Every pre-existing file's extracted SHA-256 is unchanged. Only the
  expected new executable and Bounce log were added. MBR, loader gap and VBR
  are byte-identical. No supervisor fault was observed.

All 2319 original inventoried files and modes remained unchanged after testing;
HEAD and refs are unchanged. This source report is the only intentional added
repository document beyond the user's initial instruction file. No generated
objects, images, logs or compiler outputs were placed in the original checkout.

## FAT32 research boundary and next implementation work

Read-only historical evidence is available under
`/data/projects/qdos386.old/reference/qdos386/r6-single-disk-internal/stage-b/`:
`HANDOVER.md`, `FAILURE.json`, `b1-from-accepted-a.patch` and
`fat32-b1-INCOMPLETE-source.tar.gz`. None was restored over current source.

The archived guest failed with supervisor EIP 0: both new read-only FAT32
driver descriptors had NULL `close`. Current `source/kernel/channel.c` still
calls `close` unconditionally in explicit close and owner cleanup. This confirms
the relevant ABI requirement; it is not a fresh reproduction or an accepted fix.
The current checkout contains no FAT32 driver. A future read-only driver needs
its own valid no-op close and negative callback-registration tests; writable
WIN1 cleanup must not be reused blindly.

Stage A1 must first resolve the native block-device interface: the current IDE
driver has one immutable boot-selected device and the worker/cache keys assume
that device. Utilities cannot safely reach a secondary disk by changing the
global selector without controller ownership, disk identity, bounds, mounted
volume refusal and cache coherence. Existing FAT16 sector callbacks and MBR
structures should be reused. No destructive utility or FAT32 mutation is yet
implemented, tested or accepted. All later gates remain pending.
