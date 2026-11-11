# QDOS/386 Apollo 16 Phase 2B: bounded sector cache (release)

Branch `r20-apollo16-io-workers`, commit `93fd6a5`, tag `a16-gate2b`, on top of `a16-gate2a` (`cf1cbcc`). Phase 1
(`a16-gate1`) and Phase 2A (`a16-gate2a`) are unchanged. The R18 frozen kernel is byte-identical to the frozen tree.

## What is here

- `source/`: the Phase 2B tree (kernel, platform, SDK, tools, the kernel test sources `tests/*.c|h` and `tests/a16/`, the
  non-private application sources the PC build links, the Apollo 16 scripts, and the design and results documents).
  Built from the tag, not from a working copy. The SuperBASIC, Infocom and story trees are not included.
- `patch/phase2b.patch`: the diff from `a16-gate2a` to `a16-gate2b`.
- `binaries/`: `r18-vga.img` (frozen R18 kernel, for comparison), `r20-2a-vga.img` (Phase 2A), `r20-2b-vga.img`
  (Phase 2B, default cache size), and the protected test programs (`R20*P.aout`): `R20GS/GW/GL` (cache workloads), `R20X`
  (contention reader), `R20XK` (1000-read reader for termination tests), `R20K` (process termination helper), `R20C`
  (write coherence), `R20D` (state dump), `R20P` (background progress).
- `performance/`: the matrix table (`mx-summary.txt`), the serial logs of the 18-run matrix, and the contention, injection,
  termination, coherence and reboot runs.
- `docs/A16-CACHE-DESIGN.md`, `docs/A16-CACHE-RESULTS.md`: the design (including the reservation rules) and results
  (gates, performance, Phase 2A diagnosis, limitations).

## Rebuild and check

    tar -xzf qdos386-APOLLO16-R20-PHASE2B-CACHE.tar.gz && cd qdos386-APOLLO16-R20-PHASE2B-CACHE
    sha256sum -c SHA256SUMS
    cd source && . <your toolchain env> && make qdos386-r20-pc      # the Phase 2B kernel (QDOS_R20)
    make qdos386-r18-pc                                              # the frozen R18 kernel (must be byte-identical to binaries/r18-vga.img)

The test programs are built by `scripts/a16-p2b-build.sh` (needs the same toolchain). The test volume is built by
`scripts/a16-p2b-sysvol.sh` from a base volume that is NOT part of this release.

## Running the diagnostics

Use the debug port, not the VGA text capture. Boot the kernel with `isa-debugcon` mapped to 0x402:

    qemu-system-i386 -machine pc -cpu pentium -m 16 -drive file=kernel.img,format=raw,if=ide,index=0 \
      -drive file=vol.img,format=raw,if=ide,index=1 -smp 2 -display none \
      -chardev file,id=dbg,path=serial.txt -device isa-debugcon,iobase=0x402,chardev=dbg

Set `/ETC/STARTUP.CMD` to one of: `START /BIN/R20X` (x8), then `/BIN/R20D` (state dump after 40000 ticks); or
`/BIN/R20C` (write test; run again with `K` for the reboot readback); or `START /BIN/R20XK` (x8), `/BIN/R20K <ids>`.

## Gate status

Passed: contention (8 readers, 8 entries, 2 and 8 CPUs; default size), forced fallback, injected device failures
(default and 8-entry), process termination during queued and waiting fills, write coherence, reboot readback, R18
byte identity, the 18-run performance matrix. See `docs/A16-CACHE-RESULTS.md`.

Not done: the Phase 2A cause is not confirmed (the diagnostic copy was built but not run); the 8-entry injected run
masks 10 device failures behind successful retries (documented); cold streaming reads are slower with the cache.

## Not included (public-safe)

Private Elite, SuperBASIC and Infocom material, story files, the base user volumes (the test volumes are built from a base
that is not in this release), and the 700 MB of run logs. Nothing here needs those.
