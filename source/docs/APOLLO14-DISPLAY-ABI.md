# Apollo 14 (R18) — display ABI additions

R18 extends `QT_SURFACE` (trap 0x3B) compatibly:

* `SURF_VERSION` stays 1, and every R13, R13A and R17 call keeps its number and meaning.
* Binaries built for earlier kernels run unchanged.
* Programs built with the R18 header run on earlier kernels; the new requests are refused there with `-4`.

## Modes the presenter drives

| Physical mode | Used for | Since |
| --- | --- | --- |
| VGA text 80×25 (mode 3) | text sessions, the Task Manager | R12 |
| 320×200×256 (mode 13h) | INDEX8 surfaces: 1:1 if ≤ 320×200, else `FIT` (scaled, aspect kept, centred) or `CROP` (320×200 window) | R13 / R13A |
| 640×480×16 (mode 12h) | rendered text consoles (`CONSOLE GFX`) | R13A |
| **640×480×16 (mode 12h)** | **INDEX8 surfaces with view `VGA12` / `VGA12W`** | **R18** |

## Views (`SURF_OP_VIEW`, op 30: b = view, c = vx | vy << 16)

| b | Name | Presentation | c |
| --- | --- | --- | --- |
| 0 | `GS_VIEW_FIT` / `QD_VIEW_FIT` | mode 13h, scaled to fit | 0 |
| 1 | `GS_VIEW_CROP` / `QD_VIEW_CROP` | mode 13h, 1:1 window at (vx, vy) | origin |
| **2** | `GS_VIEW_VGA12` / `QD_VIEW_VGA12` | mode 12h, **1:1, centred**: x0 = (640 − w)/2, y0 = (480 − h)/2 | must be 0 |
| **3** | `GS_VIEW_VGA12W` / `QD_VIEW_VGA12W` | mode 12h, each pixel **2× wide** (if 2w ≤ 640, else 1:1), centred | must be 0 |

Every surface size R13A allows (up to 512×256) fits 640×480 without scaling rows. For a 256×256 surface such as
EliteQL's, VGA12 places it at (192, 112), and VGA12W shows 512×256 at (64, 112), which is how a QL displays its mode 8
screen.

**Colour policy (views 2, 3).** The displayed colour of a pixel value *v* is palette entry *v & 15*: DAC registers
0..15 hold palette entries 0..15 (8-bit RGB >> 2). Entries 16..255 are kept, and returned by `PAL_GET`, but not
shown. Values 16..255 appear as their low four bits. The border is entry 0. Programs with up to 16 colours (EliteQL
uses 8) are shown exactly.

**Capability.** `SURF_OP_CAPS` returns bit `SURF_CAP_VGA12` (0x40) when the display can do views 2 and 3 (VGA builds
of R18). The view is refused with `SURF_ERR_ARG` (-4) where it is not available (headless builds, or R17 and earlier),
and also when c ≠ 0. A program can fall back to FIT:

```c
if (qd_gfx_view(QD_VIEW_VGA12, 0, 0) < 0)
    qd_gfx_view(QD_VIEW_FIT, 0, 0);
```

## User presentation override (`SURF_OP_SESS_VIEW`, op 37)

The user can choose how a session is shown without changing the program, which is how the frozen EliteQL is shown at
full height.

* **Call**: `qtrap(0x3B, 37, session, c)`.
  * c = 0: the program's own view.
  * c = 1 + view, for FIT (1), VGA12 (3) or VGA12W (4).
  * c = 0xFFFFFFFF: query.
  * Returns the previous override.
* **Errors**:

  | Return | Meaning |
  | --- | --- |
  | `SURF_ERR_STALE` (-2) | no such session |
  | `SURF_ERR_ARG` (-4) | bad c, or VGA12/VGA12W where the display cannot do it |
  | `SURF_ERR_OWNER` (-8) | not allowed |

* **Who may set it**: a job on the session, or a caller that (itself or through its parent) created the session or is
  the parent of a job on it. A tool started from the same shell as the program therefore may. Only presentation
  changes: no pixel or palette entry is read or written, and the program is not told.
* **Lifetime**: the override belongs to the session and is cleared when the session index is reused.
* **User interface**:
  * The Task Manager (Ctrl+Esc) **F3** cycles the selected job's session: program's choice → 640×480 1:1 → 640×480
    2× wide → 320×200 fit → program's choice.
  * `/BIN/VIEW con [APP|FIT|VGA12|VGA12W]` does the same from the shell; the CON column of `TASKS /L` gives `con`.

## Presenter cost (`SURF_OP_PRES_TIME`, op 38)

`b` is an 8-word user buffer. It receives: calls, drawing calls, the maximum TSC cycles of one drawing call, total
cycles (low and high words), the maximum rows in one call, full redraws, and rows drawn. The presenter runs with the
big kernel lock held and interrupts off, so this is its share of lock time. `PLANAR TIME` prints it in µs.

## Ownership, focus and lifetime (unchanged from R13/R13A)

* A surface belongs to the job that opened it, and only that job draws or changes its palette or view.
* Only the foreground session is presented and receives keys. A hidden session keeps drawing into its own surface and
  never touches the VGA.
* Each session's pixels, palette, cursor and view are private. The presenter redraws the whole selected surface, with
  its own palette, on every focus change.
* The Task Manager overlay switches to text mode and is redrawn over any session. Closing it restores the session's
  physical mode.
* A surface ends with `GFX_CLOSE` or with its job (exit, kill, fault), and its session then returns to text.

## SDK (`sdk/include/qdos386.h`)

`QD_VIEW_VGA12`, `QD_VIEW_VGA12W`, `QD_SURF_CAP_VGA12` and `QD_SURF_OP_SESS_VIEW` are header-only additions. The
library is unchanged, so earlier binaries and the SDK's library objects are byte-identical.

## Demo

`/BIN/PLANAR` (source `tests/a14/planar.c`) is a public test card. It covers 16 colour bars, the first and last rows
and columns, the corners, both diagonals, a checkerboard, odd-x/odd-y singletons, values ≥ 16, a moving bar and a
"dashboard" band in the last 64 rows. It runs in any size from 32×128 to 512×256 and any view, and each run dumps the
surface for the host oracle.
