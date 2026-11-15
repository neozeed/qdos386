# R7 canonical source/build consolidation — PASS

The accepted R7B2 OS is now the sole production build. **No R8 work.**
Branch: `r7-canonical-build`. Implementation checkpoints: `7f2089a` and
`c813f2a`; final acceptance tag: `apollo-r7-canonical-build`.
Baseline `dafefb62fbc21f2753bca0cc48972805fdab1c09` and every existing accepted
branch/tag are preserved. The kernel implementation and historical application
sources are unchanged. Three original untracked instruction files remain intact.

`make` builds project-owned GCC 1.40/CCCP, GAS 1.38 and LD 1.94, then the current
kernel/bootloader, matching shell/SH, fresh SDK libraries, FDISK/FORMAT/CHKDSK
and a bootable FAT16/FAT32 system disk. Everything generated lives in
`build/current`, or an explicit `BUILD_DIR`. `make clean` removes only that
marked tree; old build directories, tracked source and private assets survive.
Historical kernel profiles are accessible through their frozen tags.

The repaired host GCC source is imported byte-for-byte from its accepted
`9491134ab6696591fc726dc1d81446d65b8de502` checkpoint, including the aggregate
correction. Original compiler history/binaries remain intact. Missing LD
`include/aout` headers were recovered from the preserved R5 tree and match the
original manifest. Runtime gnulib, C/math/QDOS libraries and all required parser/
backend inputs are owned explicitly. No build-time source patching remains.

| Acceptance | Result |
| --- | --- |
| Fresh checkout, default build | PASS, no private source/toolchain paths or prebuilt SDK inputs |
| Rebuilt host historical tools | PASS, identical hashes across different checkout/output paths |
| Accepted comparisons | 14 PASS: serial/VGA kernel packages, VGA kernel a.out, boot3, shell/SH, three utilities, four SDK libraries and full private disk |
| Source-only disk runtime | FAT32 writes, owner cleanup, shell/SH and FDISK PASS; independent FAT16/FAT32 checks clean |
| Full private disk runtime | Above plus native GCC/HI, Bounce, Hack, Elite and graphics switching PASS; native HI exactly matches accepted bytes |
| Clean | PASS, no generated outputs or source changes remain; four unsafe destination refusals verified |
| Rebuild determinism | All 37 recorded binary/library/image products match, in both minimal and full-private modes |
| Dependency trace | PASS, no old hosttools, private compiler tree or prior temporary workspace reads during source-only rebuild |
| Compiler correction retained | Exact source import plus minimal aggregate compilation at both supported optimization settings PASS; no 630-file matrix repeat |

Reproducible host cc1 SHA-256:
`58e0a0cf4b73ac14cfb4af15292b27c9017ea25a246e4149dec17681d111a6e3`.
The host executable has normalized debug/macro paths; it is intentionally not
claimed to equal the earlier `06371b4e...` ELF binary. All target comparisons
pass. Host GCC is Ubuntu `15.3.0-4ubuntu1`; [ownership](../../toolchain/OWNERSHIP.md)
and [build guide](../../docs/BUILD.md) list required host tools. Full tool hashes
are recorded in the retained `evidence/host-toolchain.json` and `SHA256SUMS`.

Default source-only disk SHA-256:
`4ce6b099abaf5540f92e14f0a38f7d649c7789fecb4c964e70d17c1956a3cdb4`.
VGA kernel package SHA-256:
`cc3466efd94699f1e61ccd7d6b5a99a10a0aeb55d9f1d2e5f95d088223305059`.
Full private disk SHA-256 (exact accepted R7B2 oracle):
`34ec7a9a0fc8dbb4f43bedf72de9fb2672d4193b3e5b725cdeab2ecde4d56074`.

The fully populated disk requires explicit
`PAYLOAD_IMAGE=/path/to/fat16-fallback.img`, SHA-256
`3b3ceb41210c9726af2cd65c55225a5571f9df7193c96d722ce017506560396a`.
This relocatable, read-only input supplies reused guest GCC binaries, historical
games/interpreters/assets and regression fixtures. The default source-only disk
needs none of it. We do not claim every historical application/asset is rebuilt
from original source. Existing resources remain preserved in the private tree.

Evidence and pristine test disks:
`/tmp/qdos386-r7-canonical-dqnq2r1o/`, especially
`evidence/canonical-private-pristine.img`,
`evidence/canonical-minimal-pristine.img`, `accepted-comparison.json`,
`determinism-*.json`, `clean-result.json`, `dependency-audit.json`,
`minimal-smoke/result.json`, `private-smoke/result.json`, `preservation.json`,
`git-final.txt` and `SHA256SUMS`.
Use a new disk copy or QEMU snapshot mode. Exact commands are in the build guide
and logs. [Build audit](R7-CANONICAL-BUILD-AUDIT.md) explains source ownership and
retired assumptions. No packages installed, remote push, publication, destructive
cleanup of historical resources, or history rewriting occurred.
