# R3 accepted canonical baseline — freeze receipt

Verified 13 November 2026 before starting R4. The existing annotated `r3-gate4-accepted` serves as the unambiguous acceptance tag; no redundant release tag is created and no accepted tag is moved.

- Tag object: `1dde2980cfe217afcb6ec24151c02bd9d69330df` (unsigned).
- Peeled/verified source commit: `3900b200ca182c445db0ba3d64c28beadc7bed02`.
- Archived source commit: `c16fa0a9fcac1c8f27b44ffeb6e4bebb5900deea`, tag r3-gate4-source; kernel/application source is identical at the receipt checkpoint.
- Internal archive exists locally; SHA-256 `c27c7b9ae73cfb493c10d7eb2c840f67d54030d34cf7fbd6092c8b7521965b6b`. Its full artifact manifest verifies.
- Frozen R2 tag object `11c67847a2b058a116d93f478a7b90e116c8aa74`, peeled `6ddc1bfc008da55b7b722c0513ebf23aa682db78`; unchanged.

## Newly executed checks

Fresh Git export built with `make -B production diagnostic r18 HOSTTOOLS=/data/projects/qdos386/hosttools`. All three new-composition disks match the R3 internal handoff manifest; all 124 R18 outputs and the separate preserved historical 250-output inventory match.

| Image | SHA-256 |
|---|---|
| canonical/pc/qdos386.img | cc2ef4a8df6d6ce7e8728c069adae4ae809b7d04c537ab3acd103b3f33ac2c21 |
| canonical/pc-vga/qdos386-vga.img | a95988315f4558114d2869f71f40f9953d050a055d61479530e03554de6a927f |
| diagnostic/pc/qdos386-diagnostic.img | b7438c8cf8914d47aa4700fd196b883795218db99e28ece7d135afbcc0b1838a |

New bounded QEMU TCG smokes: 16 MiB VGA at 1/8 CPUs and serial at one CPU, normal shell/IOW0/idle topology, CCCP/CC1/AS/LD and HI. Explicit diagnostic composition runs original R4–R8 criteria with no individual FAIL. Disks are copies and references remain unchanged. Exact QEMU argv, tool versions, compile/link commands/flags and evidence hashes are in `reports/r4/evidence/r3-freeze.json`; raw records are ../test-runs/qdos386/r4-freeze-*.

Production main still enters shell_boot; no legacy test objects or diagnostic range bridge are linked. Diagnostics retain intact original test bodies and the QDOS_LEGACY_DIAGNOSTIC-only bridge. No production pointer relaxation is introduced.

The broader R3 evidence (18 runs, 1007 checks, 79628 PTE checks) is previously recorded, not newly executed here. Accepted scope remains nine relocated diagnostic pairs, production/diagnostic separation, 53 selected C decisions and current contracts/inventory. Not every historical branch was removed; numeric bounds, user ABI, applications/SDK and cache policy remain unchanged.

Private development policy is preserved at repository root. R4 starts from the accepted R3 commit on its own branch. No installation, merge, push, publication, resource deletion or tag rewrite is performed.
