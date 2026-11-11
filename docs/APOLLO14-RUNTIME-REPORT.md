# Apollo 14 (R18) — runtime report

**Setup**

* QEMU `pc`, `-vga std`, SeaBIOS, TCG (`thread=multi` where SMP), on a 2-core host. One emulator at a time.
* Kernel: `build/qdos386-r18/pc-vga/qdos386-r18-vga.img` (final build: SHA-256 prefix `d42497d5`).
* Volume: the public-safe R18 system volume (`scripts/a14-sysvol.sh`).

**How pixels are checked**

* Every 640×480×16 picture below was compared pixel for pixel by `tests/a14/oracle.py`, using a dump of what the
  kernel's surface held. The dump comes from the program (`qd_gfx_read`) or from `%gsnap` (guest memory, guest
  stopped).
* The model covers layout, colour & 15, bitplane packing and QEMU's DAC. "0 differ" means all 307,200 pixels are equal.

**Results**: PASS, FAIL, NA or NOT TESTED as stated. Final-build logs are in `logs/a14/final` (`evidence/` in the
package).

## Gate 0 — baseline (R17)

R13A gate 2 (rendered text consoles, mode 12h): 26 checks, 0 failed. R13A gate 3 (512×256 / 320×200 surfaces, FIT/CROP,
pixel-exact): 22 checks, 0 failed. **PASS**

## Gate 1 — planar presenter (`tests/a14/run-s1.sh`, 2 CPUs)

| Card | Layout | Result |
| --- | --- | --- |
| 256×256 VGA12 (EliteQL size) | (192,112) 1:1 | 0 differ |
| 256×256 VGA12W | (64,112) 2× wide → 512×256 | 0 differ |
| 512×256 VGA12 | (64,112) | 0 differ |
| 320×200 VGA12 | (160,140) | 0 differ |
| 37×131 VGA12 (odd x, odd y, odd size) | (301,174) | 0 differ |
| 320×200 VGA12W (the full 640 width) | (0,140) | 0 differ |
| 511×255 VGA12W (too wide to double → 1:1) | (64,112) | 0 differ |
| palette changed 3× after drawing | (192,112) | 0 differ |
| 256×256 VGA12 again, after a mode 13h program | (192,112) | 0 differ |
| 256×256 FIT | mode 13h kept (QEMU dumps 640×400) | PASS |

What the cards contain:

* all 16 colours;
* the first and last row and column, and the 4 corners;
* both diagonals, a checkerboard, and odd-position singletons;
* values ≥ 16 (shown & 15);
* text and a 64-row "dashboard" band.

Between cards the display goes back to text mode, so every card also exercises a text → 12h → text switch. The same 9
cards pass on 1 CPU and on 8 CPUs (Gate 4).

`PLANAR CAPS`:

* `SURF_CAP_VGA12` is set.
* `VIEW` refuses c ≠ 0 and b = 4 with -4, and accepts 2 and 3.

R13A regression gates on the R18 kernel: gate 2 26/0 and gate 3 22/0, so 320×200×256 and the rendered text consoles
are unharmed. **Gate 1: PASS**

## Gate 2 — session switching (`tests/a14/run-g2.sh`, 2 CPUs): 20 checks, 0 failed

The shell's console is rendered in 640×480×16 (`CONSOLE GFX`). Three graphics jobs run alongside it:

* PLANAR A1, 256×256 VGA12;
* PLANAR A2, 200×160 VGA12W;
* BOUNCE, 320×200×256.

Both PLANARs animate, change their palette every 64 frames, and keep drawing while hidden. Sessions were switched by 7
Task Manager selections and 10 Alt+F1 returns to the shell, and the overlay was also opened over mode-12h graphics
sessions.

* A1 was checked 4 times and A2 3 times. Every check was pixel-exact against its own surface: no cross-session pixels,
  and the private palette was restored each time.
* Each shell screen was **identical** to the first one, and used only the 16 text colours.
* BOUNCE was shown in mode 13h each time, and its self-check reported `verify=PASS`.
* A2 was ended from the Task Manager (F1, Y) while A1 kept running. A1 and BOUNCE ended by themselves.
* LEAK was back to baseline, with no PGFAULT. **PASS**

## Gate 3 — EliteQL at full height (private fixture; `tests/a14/run-elite.sh`)

The frozen protected `ELITEP.AOU` (SHA-256 `50b64c18…`, from the R16B Elite package) was used **unchanged**.
Before the override, Elite asks for FIT, which is the R17 picture: mode 13h, 56 of 256 rows lost. Task Manager **F3**
switches it to 640×480×16 1:1; F3 again gives 2× wide; `VIEW 5 VGA12` from the shell does the same for a second Elite.

**2 CPUs, two Elites:** 12 snapshot pairs, each the screen plus Elite's surface.

* Paused (F5) or static screens, **9 of 9 pixel-exact**, all 256 rows on screen and the dashboard intact:
  * title-menu pages after N, X, 6 (market/status) and 8;
  * flight, paused;
  * the rear view, paused, at 1:1 and 2× wide;
  * the second Elite in flight, paused;
  * the first Elite after switching back, paused.
* Moving screens (the title's rotating ship, the death screen): 4–756 pixels differ. The presenter is up to a few
  rows behind the surface (no vsync, at most 64 rows per tick); this is expected.
* Both Elites were in the Task Manager. With the shell focused, both background Elites were on CPU 1 (TOP: CPU1
  load=2); the focused Elite runs on CPU 0, as in R16B foreground-first.
* Both were ended from the Task Manager, LEAK went back to baseline, and there was no PGFAULT. **PASS** (0 failed).

**8 CPUs (4 cores × 2 threads), one Elite, bounded smoke:**

* The VGA12 title screen differed in 462 pixels (it is moving).
* The second snapshot is static and is **pixel-exact**. The 8-vCPU guest on 2 host cores missed the launch key holds,
  so this snapshot shows the status page, not flight.
* LEAK baseline, no PGFAULT. **PASS** (smoke).

The QL packed framebuffer → index conversion is Elite's own unchanged adapter code; it was verified in R16B. R18
checks surface → screen. **Save/load in R18: NOT TESTED** (unchanged game code, not a presentation concern).

## Gate 4 — compatibility and robustness (`tests/a14/run-s4.sh`; 1 CPU and 8 CPUs)

| Check | 1 CPU | 8 CPUs (4c × 2t) |
| --- | --- | --- |
| Stage 1's 9 cards, pixel-exact | 9 / 9 (0 differ) | 9 / 9 (0 differ) |
| FIT card stays in mode 13h | PASS | PASS |
| `CAPS` bit; `VIEW` c ≠ 0 and b = 4 refused (-4) | PASS | PASS |
| Override: query (APP), VGA12W → FIT → APP each reporting the previous value | PASS | PASS |
| Malformed: session 99 (-2), unknown mode name (tool), c = 2 (-4) | PASS | PASS |
| A sibling job may set and clear an override | PASS | PASS |
| An unrelated job (the shell's grandchild) is refused (-8) | PASS | PASS |
| Low memory: 64 KiB + 3 × 128 KiB asked for against the 400 KiB pixel budget; the 4th is refused cleanly and 3 run | PASS | PASS |
| 20 × (open 512×256, VGA12, draw, close): pixel memory back, no failure | PASS | PASS |
| Animating PLANAR ended from the Task Manager; no PLANAR left | PASS | PASS |
| LEAK baseline, no PGFAULT, no harness timeout | PASS | PASS |
| fsck of the final volume | clean | clean |

Both runs had 0 failed. On 8 vCPUs the harness spaces typed keys 0.3 s apart; with 0.12 s, characters were lost while
a background job animated. This is QEMU `sendkey` timing on an overloaded 2-core host; the 1-CPU run with 0.12 s lost
none. **Gate 4: PASS**

## Presenter cost under the BKL (`PLANAR TIME`, TSC in µs; TCG on a 2-core host)

| Run | Drawing calls | Avg µs | Max µs | Max rows per call |
| --- | --- | --- | --- | --- |
| Stage 1 cards, first build (no bound; 4-pass `put_line`) | 14 | 33,805 | 69,878 | 256 |
| Stage 1 cards, final build (64-row bound, 1-pass gather) | 55 | 4,071 | 20,211 | 200 (mode 13h FIT card) |
| Robustness, 1 CPU, final | 128 | 2,692 | 7,623 | 160 |
| Elite, 2 CPUs, final (7,202 drawing calls, 562,592 rows) | 7,202 | 3,869 | 42,151 | 200 (mode 13h title before F3) |
| Robustness, 8 CPUs, final | 281 | 32,034 | 152,356 | 200 |

* **Bounded work:** a mode-12h call draws at most 64 rows. The largest single calls are the unchanged R13A mode-13h
  path (up to 200 rows), which R18 does not alter.
* **The 8-CPU figures are inflated:** they include host descheduling of the CPU 0 thread (8 vCPU threads on 2 host
  cores). The TSC measures wall time.
* **On real hardware** the TCG figures overstate the cost by a large factor. Not measured on hardware.

## Frozen outputs

* Unchanged: R17 `40968de4` / `f53bd88a`, R16B `d073fbea`, R16A `b9ecb69a`, KCTX-FIX2 `f8e5d5e1` / `7c675978`.
* `make -B` in the package script, plus a fresh-package rebuild (`scripts/a14-verify-package.sh`).

## Not done

* VBE/SVGA, real hardware, vsync.
* Tiny386 (no VGA output in that build).
* The thousand-switch Elite stress (not rerun, as instructed: no concrete regression required it).
