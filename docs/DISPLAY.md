# Display, input and session contract

> Preserved R3 baseline document. For current R4 resource changes and profile status, see [R4 configuration](R4-CONFIGURATION.md), [ABI review](R4-ABI-REVIEW.md) and [known limitations](R4-KNOWN-LIMITATIONS.md). The R3 text below is retained for provenance.

`vcon.c` owns text sessions, foreground routing and waiters; `surface.c` owns optional session graphics and references; `gsurf.c` provides INDEX8 drawing; fontreg/font_console own font references/rendering; `platform/pc/vgapres13a.c` owns VGA/planar presentation. Only the foreground session is presented and receives keys. Shared pixel/state access uses the existing kernel serialization; presentation/free lifetimes must not overlap incorrectly.

Native surface calls retain QT_SURFACE=0x3B, version/suboperation IDs, argument records and return conventions. A surface belongs to a session, with existing owner/creator/parent permissions. Graphics open allocates a bounded framebuffer; close restores session text; release is deferred when necessary until the presenter/system poll can free safely. Four graphics surfaces and twenty sessions are unchanged. Palette/font generations and frame counters remain observable through existing calls.

The current VGA branch includes 640x480x16 planar presentation and the accepted session view overrides; older 320x200/indexed helpers remain where meaningful. Serial/headless builds preserve software surfaces and diagnostic output without claiming hardware presentation. R3 resolves selected old display branches only where the current production/R18 implementation is identical; the diagnostic kernel does not link those graphics services.

i8042/kbd handle IRQ scancodes. kev.c supplies per-session key-down/up events; hotkeys are consumed, focus loss synthesizes releases, and orphan key-ups are dropped. Alt+Fn/console commands change focus; Ctrl+Esc opens the Task Manager overlay. taskmgr.c records/restores previous focus and wakes SYSTEM polling. The overlay is not an ordinary user surface or a new scheduler.

Kernel Task Manager rows use JOB_MAX; legacy VCON per-job tracking still covers only sixteen slots. Shell enumeration/history capacities are separate. Do not infer a display/task-table fix from branch cleanup. Actual session switching, overlay state, graphical fixture initialization and cleanup are tested; manual long gameplay, new display modes and arbitrary hardware are not claimed.
