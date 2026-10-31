# Tiny386 for QDOS/386 R13A (unchanged from R12C and R13)

Upstream Tiny386 with `patches/tiny386-fix1-shutdown.patch` (guest 0x8900 shutdown) and
`patches/tiny386-r12c-flush.patch` (ATA FLUSH CACHE).  `src/` holds the changed files, `bin/` the binaries built
with `make USE_SLIRP=n tiny386_headless tiny386` (docs/R11M-FIX1-EMULATOR-BUILD.md).
