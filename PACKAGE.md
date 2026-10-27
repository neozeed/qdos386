# QDOS/386 R11E package

Source commit: 7a62af13c0fdfd971a6be1a03dc15815003c9c6a (branch r11e, from the frozen R11D checkpoint 2d409fb).
Contents: source tree (git archive of the commit), built SDK libraries and example
applications, disk images, i386VM shell image, run/ directories, SHA256SUMS.
FAT16 volume (images/win1-r11e.img) sha256: b36a52aee33facf916136e78f70181db402a9046bd6672017f914c9480acb05c

Run (interactive VGA, Tiny386): cd run/interactive && tiny386_headless interactive.ini
Run (diagnostic, Tiny386): cd run/diagnostic && tiny386_headless diagnostic.ini

Evidence and limits: docs/R11E-RUNTIME-REPORT.md, docs/R11E-KNOWN-LIMITATIONS.md,
docs/R11E-LIBQDOS-ABI.md, docs/R11E-QDOS-API-MAP.md, docs/R11E-C89-COVERAGE.md.
All runs were scripted.
