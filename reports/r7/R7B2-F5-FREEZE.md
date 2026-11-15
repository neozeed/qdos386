# R7B2 F5 reproducibility and freeze — PASS

The F4 source tag `apollo-r7b2-fat32-system` (`a8c0706`) was freshly extracted
with `git archive` under `/tmp/qdos386-r7b2-fat32-glru8dtm/f5-freeze/source/`.
The final F5 commit changes documentation only. All source builds use the
accepted repaired historical compiler; no archived objects replace missing
outputs. `build-verification.json` records 13 exact comparisons: writable VGA
package/kernel a.out, shell/SH, three disk tools, two native probes, full combined
disk, and the read-only VGA package/shell/SH. Every comparison passes.

The new disk is independently decoded, every populated file hash compared,
and both partitions pass `fsck.fat -n`. It is byte-identical to the already
boot-qualified F4 master; no redundant runtime matrix was rerun. Linux tools
are unmodified. No source or application assets were removed.

## Reproduce in a new temporary directory

These commands assume the existing pinned input paths are available. All output
paths must be new paths under `/tmp`. The private fallback provides existing
historical payloads; it is read only. `fat32-combined.py` preserves their bytes
and long names and refuses to reuse its output directory.

```sh
work=$(mktemp -d /tmp/qdos386-r7b2-rebuild-XXXXXXXX)
tools=/tmp/qdos386-r7b2-fat32-glru8dtm/hosttools
mkdir "$work/source" "$work/tmp" "$work/products"
git archive --format=tar --output="$work/source.tar" apollo-r7b2-fat32-system
tar -xf "$work/source.tar" -C "$work/source"
export TMPDIR="$work/tmp" QDOS386_TOOLS="$tools"
make -C "$work/source" apollo-r7-rw disk-tools HOSTTOOLS="$tools" >"$work/build.log" 2>&1
"$work/source/source/sdk/bin/qdos386-gcc" -O -DQDOS_R20 \
  -I"$work/source/source/kernel" -o "$work/products/F32IO.AOU" \
  "$work/source/validation/r7/fat32-native-channels.c"
"$work/source/source/sdk/bin/qdos386-gcc" -O -DQDOS_R20 \
  -I"$work/source/source/kernel" -o "$work/products/F32LIVE.AOU" \
  "$work/source/validation/r7/fat32-native-live.c"
cc -std=c89 -Wall -Wextra -Werror -DQDOS_FS_WRITE -DQDOS_VFAT \
  -DQDOS_APOLLO_FAT32 -DQDOS_APOLLO_FAT32_MUTATION \
  -I"$work/source/source/kernel" \
  "$work/source/validation/r7/fat32-populate.c" \
  "$work/source/source/kernel/fat16.c" -o "$work/products/populate"
python3 "$work/source/validation/r7/fat32-combined.py" \
  --output "$work/combined" \
  --fallback /tmp/qdos386-r7-1wn7185l/checkpoint/fat16-fallback.img \
  --package "$work/source/source/build/apollo-r7-rw/pc-vga/qdos386-vga.img" \
  --apps "$work/source/source/apps/disktools" \
  --channels "$work/products/F32IO.AOU" --live "$work/products/F32LIVE.AOU" \
  --populator "$work/products/populate"
```

Dependencies: pinned GCC 1.40 cc1/cccp, GAS, historical i386 linker and wrapper
under the external tools path; available host C compiler, GNU Make, Python 3,
mkfs.fat/fsck.fat and QEMU. The previously preserved compiler source and rollback
toolchain remain under `/data/reference/qdos386/` and the original tools path.
No packages were installed. Host tool hashes are in `SHA256SUMS` and the compiler
source provenance remains in the accepted Part I reports/bundle.

## Manual boot without modifying the accepted disk

```sh
TMPDIR=/tmp/qdos386-r7b2-fat32-glru8dtm/f5-freeze/tmp \
qemu-system-i386 -machine pc -accel tcg -cpu pentium -m 32 -smp 4 \
  -vga std -nic none -snapshot \
  -drive file=/tmp/qdos386-r7b2-fat32-glru8dtm/f5-freeze/combined/qdos386-r7b2-fat32.disposable.img,format=raw,if=ide,index=0 \
  -boot order=c -no-reboot
```

At the shell, `RUN GCC -o /TMP/HI.AOU /HOME/HI.C`, then `RUN /TMP/HI.AOU`.
`RUN SH` enters the interactive shell; `EXIT` returns. `START ELITE /new`
starts Elite; select its window with Ctrl+Esc. Answer title prompts if shown
(N then X in the recorded automation), then backslash, Q, Y to quit. Automation
used 300 ms key holds; no game modifications were required.

## Frozen artifacts and preservation

All paths below are relative to
`/tmp/qdos386-r7b2-fat32-glru8dtm/f5-freeze/`:

- `combined/qdos386-r7b2-fat32.disposable.img`: pristine full disk,
  SHA-256 `34ec7a9a0fc8dbb4f43bedf72de9fb2672d4193b3e5b725cdeab2ecde4d56074`.
- `source/source/build/apollo-r7-rw/pc-vga/qdos386-vga.img`: kernel package,
  SHA-256 `cc3466efd94699f1e61ccd7d6b5a99a10a0aeb55d9f1d2e5f95d088223305059`.
- `source.tar`, `build-verification.json`, `combined/receipt.json`, `logs/`:
  extraction, exact comparison ledger, payload/source hashes and full commands.
- `preservation.json`, `git-final.txt`, `source-history.bundle`, `SHA256SUMS`:
  preservation checks, final graph/refs/layout, local Git backup and selected
  immutable artifact/evidence hashes.

The original counterexample and both original fsck outputs are checked against
`f2-resume-jcqdsh61/preserved.json`. Old accepted branches/tags and image/compiler
hashes are unchanged. Apart from intentional kernel/build/test/documentation
changes, original tracked files retain their bytes/modes/symlink identities.
The only untracked repository entries remain the three original instructions.
No generated image, object, log or binary was placed in the source repository.
The latest instruction to keep generated outputs under `/tmp` takes precedence
over the older mission's proposed non-Git reference copy; no new artifacts were
copied outside `/tmp`. Existing durable compiler archives remain untouched.
