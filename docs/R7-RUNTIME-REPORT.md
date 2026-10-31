# QDOS/386 R7 — runtime report

R7 turns the R6 test kernel into a small diagnostic shell. A user types `DIR`,
`TYPE`, `RUN`, `HELP` and `EXIT` at a prompt, and the commands run from the
WIN1_ FAT16 volume. Everything in this report comes from the logs in
`logs/r7/`. Nothing here was typed by a person.

## 1. What was built

| phase | commit | result |
|-------|--------|--------|
| 0 | b092167 | audit of the keyboard path, input port, traps, slots and the FAT16 walker (`docs/R7-AUDIT.md`) |
| 1 | c9550f3 | i8042 keyboard and IRQ1 on the PC; i386VM status register (patched harness); set-1 decoder; ring; blocking CON read and `QT_CON_WAIT` |
| 2 | c23defb | root directory listing in the shared FAT16 layer; `WIN1_` directory channel |
| 3 | 28933ef | the shell (slot 0) and `QT_IMG_LOAD` (RUN into slot 1) |
| 4 | f740345, d0d9bc8 | parameterised build; shell boot images (`main()` selects shell mode with `R7_SHELL_MODE`) |
| 5 | 8475a6a, d113d24 | prompt-driven sessions, timer-off negative, verification matrix, the console-waiter fix |

The ABI is in `docs/R7-CONSOLE-ABI.md`. The shell design is in
`docs/R7-SHELL-DESIGN.md`.

## 2. How it was verified

The matrix is `scripts/r7-matrix.sh`. It runs three rounds of:

1. the R2–R7 regression suite on i386VM (patched harness);
2. the same suite on Tiny386 (PC);
3. the shell session on i386VM, through a pipe (SCRIPTED);
4. the shell session on Tiny386, through `-term` (PS/2 keyboard path);
5. the i386VM EOF session (input closed without EXIT).

After the rounds it runs the timer-off negative and records SHA-256 of every
image and input.

Input paths, labelled in every log header:

- **SCRIPTED (pipe)**: the i386VM harness reads bytes from stdin through its
  input status register (0x800004) and byte read (0x800002). The keyboard path
  is not used.
- **PS/2 via -term, driven by session.py**: each byte becomes a key through
  the emulated i8042 and IRQ1. The driver, not a person, sends the keys. Keys
  are sent only after the shell prompt appears.

Neither path is human-interactive evidence. Nothing in this release shows that
a person can type into the shell and get the right result.

## 3. Results

Final matrix, on the code at commit d113d24 (three rounds):

| check | i386VM | Tiny386 |
|-------|--------|---------|
| regression suite (R2–R7, 177 CHECK lines), 3 runs | 3 × PASS, 0 FAIL | 3 × PASS, 0 FAIL |
| shell session, 20 steps, 3 runs | 3 × PASS | 3 × PASS |
| EOF session (i386VM only), 3 runs | 3 × PASS | not applicable |
| timer-off shell negative | prompt printed, no input read, no exit (instruction limit) | not run |

The session steps (both emulators, same commands):

- `HELP`, `DIR` (ten files in directory order, with sizes), `TYPE README.TXT`
- `type readme.txt` (folding to upper case)
- `RUN HELLO.AOU` twice in the same shell, each `exit status 7`
- `DIRX` with Backspace (`DIR` runs)
- `XY?` (a shifted character, PC path)
- `BOGUS` (`unknown command`)
- `TYPE NOPE.TXT` (`no such file`), `TYPE CHAIN.AOU` (`bad cluster chain`),
  `TYPE EMPTY.TXT` (empty output)
- `RUN BIG.AOU` (too large, refused before reading), `RUN BAD.AOU` (code -21),
  `RUN EMPTY.TXT` (code -20), `RUN SLOT0.AOU` (not slot 1), `RUN NOPE.AOU`
  (`no such file`)
- `RUN HELLO.AOU` a third time
- `EXIT` (`SHELL: exited with status 0`, halt with 0)

The EOF session types `DIR` and closes the input. The shell prints
`end of input` and exits with status 3.

The R2–R7 suite prints `R4 CHANNEL PASS`, `R5 A.OUT EXEC PASS`,
`R6 FAT16 EXEC PASS`, `R7 CONSOLE PASS` and `R7 SHELL SELFTEST PASS`, with no
FAIL lines. The shell self-test has 19 checks and reports `0 failed`.

Timer-off negative: the shell image without the timer prints the banner and
`SHELL: timer disabled (negative test build)`, then the first prompt. It reads
no input and never exits, and the harness stops with `instruction limit exceeded`.
This shows that input is delivered through the timer tick.

## 4. Clean-extraction rebuild

`git archive HEAD` into an empty directory, then `make apps fat16 i386vm pc
i386vm-shell pc-shell i386vm-shell-notimer`. All six images match the working
tree byte for byte (SHA-256 below).

## 5. Hashes

Images (`logs/r7/phase5/images.sha256`):

    8edf9d0bc040e8a8df1a3b7f43bd66dcb23105c12cd8a27df4011281c22c8707  build/i386vm/qdos386-r6.bin
    bad58ca19ff8021037afd3eeb6242b92da5376c89d72be0926242c71adc88f82  build/pc/r6pc.img
    8e94ba7bfc230e7f34871197f0a0917ced985ab43462bdb48158c1e7d1e88a33  build/fat16/win1.img
    1f68fcf6ce8dd80d1572740452a3bde0d051c6b6e5e55b63cb0defed72b5d45e  build/i386vm-shell/qdos386-r7-shell.bin
    ada6eae73c9fd26217ed9923615c8c03de53d6644b7804966ceec11114ee2f2a  build/pc-shell/r7shell.img
    f82e8df20d2836a28e68e476243ad5586c57b627b513debb1c17da86c0eec010  build/i386vm-shell-notimer/qdos386-r7-shell-notimer.bin

External inputs (the trees are not in the archive; hashes as recorded in `patches/HASHES.txt`):

    7bd968419c81fcf8262c5c1182cef43bb0d5eb70f375f94567ae71fd64a15e9e  bios.bin
    f5aedb52cde5c88d85e5b304a4cc48459c231aef42b9950ccc82bd321d9c127e  vgabios.bin
    2d2f8beba177592ed5485c60b3ca32e201e7ebec339e10e7756acf9498ad0bc1  i386vm (patched, input status register)
    fb63b4057e3a62f5fadd3d532e9c553a0d8ccecd5a9a32a7523b1f228706a9fc  tiny386_headless (unmodified)

The i386VM harness in `patches/` is the only change to an external tree. The
unpatched harness cannot run the shell image: the shell reads 0x800004, which
is outside RAM on the unpatched harness. The R2–R7 regression also passes on
the patched harness, which is what the matrix uses.

## 6. Decisions

- **Banner `SHELL SELFTEST PASS`, not `INTERACTIVE PASS`.** The plan named the
  self-test `INTERACTIVE PASS`. The self-test runs scripted in the guest with
  no keystrokes, so the name would suggest a human test that did not happen.
- **Shell in slot 0, loaded by the kernel.** RUN refuses slot-0 images, so the
  shell cannot be replaced by a child.
- **One child at a time, slot 1 only.** The shell waits for each child.
- **No busy wait anywhere.** `QT_CON_WAIT` sleeps the caller; the arrival path
  only marks the waiter runnable and never schedules from inside an IRQ.
- **PC EOF is not implemented.** Tiny386 gives the guest no end of input. A PC
  session ends only with `EXIT`.

## 7. Corrections made during Phase 5

- **Second console waiter.** The first `QT_CON_WAIT` let a second job replace
  the first waiter, and the first job was then never woken. The second waiter
  now gets `CON_ERR_BUSY` (-50), and the first keeps its wait. The shell backs
  off one tick if its wait is refused. No test in the suite exercises the
  refusal. It was reviewed, not tested.
- **Probe finding.** A keyboard probe found that keys sent at process start are
  lost before the i8042 is initialised (`irqs 0 status 28`). With keys delayed
  by about 12 seconds, IRQ1 delivered them. The probe patches are in
  `tests/interactive/probe/` and apply to a throwaway copy of the tree. The
  logs are `logs/r7/phase1/pc-tiny386-keyboard-probe-*.log`. The session driver
  waits for the prompt, so it does not depend on this timing.
- **R6 PC image hash.** The R6 Tiny386 image hashed to `a3c04023…` in an earlier
  build of this tree. Rebuilt from the same sources, it hashed `e57a6091…` with
  both the original and the new Makefile, and that was the state at Phase 4 part
  1. The cause of the earlier difference was not established. The current
  R6 image (`bad58ca1…`) differs because `shellboot.o` and the shared loader
  are now linked in. The R2–R7 suite passes on both images.

## 8. Known limits and open items

- **Not human-interactive.** Everything above is scripted, or driven through
  `-term` by a script. A human session on a real or emulated console has not
  been run.
- **Console waiter refusal** is not tested (section 7).
- **`QT_CON_WAIT` flag pointer** is not range-checked, the same as the hold
  byte of `QT_SUSPEND`. A caller must pass the address of a live byte.
- **Names are at most 10 characters through `WIN1_`** (console ABI, section 9).
- **PC EOF** is untestable (section 6).
- **Single console reader.** One job may wait on the console at a time.
- **Only the root directory** is visible; the directory channel is read-only.
- **No SuperBASIC, FAT writes, VGA window manager, relocation, multiterminal or libc.**

## 9. Reproducing

    make apps fat16 i386vm pc i386vm-shell pc-shell i386vm-shell-notimer
    make build/pc/run.ini SEABIOS=<firmware dir>
    make build/pc-shell/run.ini PCD=build/pc-shell PCIMG=r7shell.img SEABIOS=<firmware dir>
    I386VM=<i386 tree> VMP=<patched harness> TINY=<tiny386_headless> FW=<firmware dir> \
      sh scripts/r7-matrix.sh

The matrix takes about 40 seconds on this machine. Its logs go to `logs/r7/phase5/`.
