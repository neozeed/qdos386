# KCTX-FIX2 native platform integration

Archive SHA256: `778a3d612d924a06f17bd8c73fb910406768264a46fdb7391bc82c36db3ed804`.
Source commit in PACKAGE.md: `b1db28aacc4171f4e1071a7adc19f6dfb735bcfd`.
Normal VGA image SHA256: `f8e5d5e1f7e5c698ed13cc57159f7c9a7faeffb8fcf523e185625a57aec0d4c3`.
Both match the handoff. The kernel is unchanged. `src/platform_qdos.s` implements
these boundaries. Full-machine Tiny386 evidence is in `R3-VALIDATION.md`; QEMU
acceptance remains unavailable.

| Original dependency | Existing SDK interface | Source and constraints |
|---|---|---|
| MODE 8 packed framebuffer / mode ownership | `qd_surface_caps`, `qd_gfx_open(256,256,QD_GFX_FOREGROUND)`, `qd_gfx_blit`, `qd_gfx_palette`, `qd_gfx_frame`, `qd_gfx_close` | sdk/include/qdos386.h, kernel/surface.c and kernel/gsurf.h; R13A supports logical 1..512 by 1..256. Private surface default; do not request SHARE. Packed game memory remains private, never physical 0x20000. |
| Physical presentation | `qd_gfx_view(QD_VIEW_FIT,0,0)` | sdk/lib/qdosgfx.c; docs/R13A-GRAPHICS-ABI.md. INDEX8 uses physical 320x200 VGA; FIT scales down preserving aspect. Logical 256x256 is supported, physical 256x256 is not required. |
| Held QL keyboard rows | `qd_key_events(1)`, `qd_key_read(events,max)`, `qd_surface_focus()` | max 64 events/read; set-1 code, E0 bit 0x100, down 0x200, synthetic 0x400, modifiers 0x1000/2000/4000. Focus loss generates synthetic releases; the adapter rebuilds eight QL rows from independent held states. |
| Foreground character queue drain | `qd_open("CON")`, `qd_con_queued(ch)`, `qd_read(ch,buffer,1)` | sdk/include/qdos386.h and sdk/lib/qdoscon.c: queued count 0..127; qd_read returns -19 when no byte is ready. The adapter drains this queue separately from held-key events. |
| 50-Hz poll clock / suspend | `qd_ticks()`, `qd_yield(ticks)` | sdk/lib/qdos.c and docs/R13B-CLOCK-ABI.md; monotonic scheduler ticks are about 100Hz (PIT 1193182/11932). The adapter retains fractional 50Hz phase and unsigned wrap; suspend passes null optional wake-flag/job arguments. |
| Snapshot allocation/free / memory query | `qd_alloc`, `qd_free`, `qd_free_bytes` | sdk/include/qdos386.h. Original snapshot/copy/free sequencing remains; the snapshot covers private game memory and closing the surface restores the OS view. |
| Commander file open | `qd_open_ex` | flags READ=1; WRITE=2, CREATE=4, TRUNC=8; FILE=0x80. Original counted filename requires conversion to a NUL-terminated path. No new save format. |
| Commander transfer/close | `qd_read`, `qd_write`, `qd_close` | returns byte count or negative error; maximum transfer 256, sufficient for existing 144/148-byte requests; retain short-transfer -10 and transfer/close precedence in R2 save.s. |
| Process exit/title | `qd_exit`, data symbol `__prog_name`, fallback `qd_set_title` | docs/R12C-PROGRAM-TITLE-ABI.md: symbol must be global DATA array, not a pointer; loader reads it. |
| QL IPC BEEP packets | No implemented QL IPC output | sdk/qlcompat/qlc_io.c returns ERR_NI for IPC; no beep/Hermes. Preserve native packet formation; explicitly unsupported adapter is acceptable per handoff. |

The adapter decodes each original packed two-byte MODE 8 pixel group into four
INDEX8 pixels using the original green/red/blue bit positions. All 65,536 words
are checked by `test-qdos-adapter.py`; the palette is the eight RGB combinations.
The original bitmap font and packed drawing routines remain unchanged. Existing
FIT presentation scales the 256x256 logical surface into physical VGA.

`_platform_poll` runs at original sound-poll/yield boundaries and only updates
clock/presentation. It does not advance the game. The actual shell owns every
frame and state transition. QL IPC output is explicitly -19, matching the
unimplemented OS facility; sound priority/packet formation still runs.

PACKAGE.md identifies SuperBASIC and the Infocom interpreter as restricted.
No OS sources, private applications or OS volume are bundled in this checkpoint.
A later public demo must use the supplied public volume only.
