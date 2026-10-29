# Tiny386 for QDOS/386 R13 (unchanged from R12C)

Upstream Tiny386 with `patches/tiny386-fix1-shutdown.patch` (R11M-FIX1: guest 0x8900 shutdown, checked image flush and
close) and `patches/tiny386-r12c-flush.patch` (R12C: ATA FLUSH CACHE 0xE7/0xEA = runtime fflush + fsync of the image
without closing it; failures reported to the guest as ERR, error register 0x40; fault injection TINY386_FLUSH_FAIL=N,
N- and TINY386_FLUSH_FAIL_STAGE=fflush for tests).  `src/` holds the changed files, `bin/` the binaries built with
`make USE_SLIRP=n tiny386_headless tiny386` (docs/R11M-FIX1-EMULATOR-BUILD.md).  See docs/R12C-SYNC-DURABILITY.md.
