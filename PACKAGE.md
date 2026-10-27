# QDOS/386 R11A package

Source commit: 0b0ad47dce6aeeab0f632763e4ec2d8d93f78087 (branch r11a, from r10-vga-frozen 9f3340a).

Contents: the source tree (git archive of the commit), the built SDK libraries and
example applications, disk images, the i386VM shell image, ready-to-run directories
under run/, logs under logs/, and SHA256SUMS for every file except SHA256SUMS itself.

Run (interactive VGA, Tiny386, external binary): cd run/interactive && tiny386_headless interactive.ini
Run (diagnostic, Tiny386): cd run/diagnostic && tiny386_headless diagnostic.ini
Run (interactive VGA, QEMU): sh launch/launch-qemu-vga.sh run/interactive 4455
Run (diagnostic, QEMU headless): sh launch/launch-qemu-diagnostic.sh run/diagnostic 4456

Rebuild the data volume from source: QDOS386_TOOLS=<GCC 1.40 tree> sh scripts/r11a-volume.sh
Expected win1-r11a.img sha256: 1a0f5d77bc181d9ef2d52c18dd6bf8e76a758d408aeda6d22fbd2f32926aad09

Tests are SCRIPTED (keystrokes or piped input). No agent-driven run is a human session.
The owner's human QEMU session is recorded only as a user-reported screenshot in
logs/r11a/user-reported/.
