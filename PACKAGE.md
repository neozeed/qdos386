# QDOS/386 R11B package

Source commit: aa44be5ac8a4e1962d5848b945e2072013b82c24 (branch r11b, from r11a 0b0ad47).

Contents: the source tree (git archive of the commit), the built SDK libraries and
example applications, disk images, the i386VM shell image, ready-to-run directories
under run/, logs under logs/, and SHA256SUMS for every file except SHA256SUMS itself.

Run (interactive VGA, Tiny386): cd run/interactive && tiny386_headless interactive.ini
Run (diagnostic, Tiny386): cd run/diagnostic && tiny386_headless diagnostic.ini

Rebuild the data volume from source: QDOS386_TOOLS=<GCC 1.40 tree> sh scripts/r11a-volume.sh <LGTEST4.aout>
Expected win1-r11b.img sha256: 030056e5d88bc1cb6a9020d6e09cbe84de02edc23014c57dd7bec8d4b3905918

Tests are SCRIPTED (keystrokes or piped input). No run in this package is a human session.
The owner's human QEMU session is recorded only as a user-reported screenshot in
logs/r11a/user-reported/.
