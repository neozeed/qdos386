# Provenance

Game translation source and ABI reference supplied by the user:

- `ELITEQL-1.0-RC6-LIVE-UNIVERSE-FULL-SOURCE.zip` SHA-256 `5a00651164f1340d0f72eee74ec0979730c6499bf0f47e0eeecd4f91fa181bb5`
- `qdos386-R12B-SYSTEM-ENV-SUPERBASIC.tar.gz` SHA-256 `af43204b3a85b170b2e68ec01b36ad3a819ca2c584d344ae8439b75555565d2f`

Assembler source: `LEGACY-GCC-HOST64-R5-MINGW64-FULL-SOURCE.zip`, SHA-256 `4a3e5419ae3b2889edf495ab3123b87cdbe5121c0d2ccb8beaaad08dae4e785f`. Only gas-1.38 C/header sources and COPYING are bundled, unchanged.

`reference/src`, `reference/tests`, and `reference/NOTICE.md` are byte-for-byte copies from the supplied EliteQL RC6 archive. The reference manifest lists their hashes. The original Makefile/assets/host binaries are not copied; this package builds the new i386 modules, not the original QL release.

GAS build flags match the legacy host64 i386 recipe: LEGACY_HOST64, USG, I386, forced host64.h include, gnu89 and fcommon. The original assembler has not been modified for this port.
