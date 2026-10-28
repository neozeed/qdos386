# Building the patched Tiny386

The Tiny386 source tree is not part of this repository. R11 was verified with the tree below; `patches/tiny386-fix1-shutdown.patch` applies to it with `patch -p1`.

Base: the Tiny386 tree used for the R11 verification (the working copy with the earlier local x87 rounding change in `fpu.c`; none of the patched files
was touched by that change). SHA-256 of the seven base files that the patch changes:

```
b77508ddf14cd688b4c0409ec268ee777218798be976175ad668bb28a8e633e2  ide.c
4b99733ab0578812deba6387aeb24e8a9cbafa60d6b4ef2072be1e336a40d36a  ide.h
ea7a0ee5123f273c5931b1039d478f71da22fd2b5fe221328795079e59a01162  main.c
a81e904c6e55c1ac314c848d168ff6986bc3eb0e7e9aadc2b886f746917a07a3  pc.c
82168df36fe64d7f8e6e09e800b98710ccf48cff44fd27f2be1f9b7d82f76595  pc.h
c7b083de9d30c621f6b2abab14efc5f58f73a04d5745cd00ac222d7d8fe24fda  rawdraw/main.c
4b391c742ae0e18126e2156c4a3fb70a88877ebd960354340636bc428017c1f8  sdl/main.c
```

```
cd <tiny386 tree> && patch -p1 < <repo>/patches/tiny386-fix1-shutdown.patch
make USE_SLIRP=n tiny386_headless tiny386        # headless runner + windowed rawdraw frontend (X11 is loaded at run time)
```

`USE_SLIRP=n` because libslirp headers are not installed in the build environment. The SDL frontend (`sdl/main.c`) is patched but needs `sdl-config`;
it was not compiled. `emulator/src/` in the package holds the seven patched files; `emulator/bin/` the two binaries built from them.
