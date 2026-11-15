# R7 canonical build consolidation — qualification in progress

Baseline: `dafefb62fbc21f2753bca0cc48972805fdab1c09`, accepted tag
`apollo-r7b2-fat32`. Safety branch `r7-canonical-safety-dafefb6`; development
branch `r7-canonical-build`. Existing references and all application sources
are preserved. The three pre-existing untracked instruction files are unchanged.

The current source/build implementation now builds the historical host toolchain,
SDK, exact R7B2 kernel/shell/bootloader, native disk tools and complete disk from
one root entry point. Source-only default and explicit private-payload image
assembly both work. The initial full private image exactly matches accepted
SHA-256 `34ec7a9a0fc8dbb4f43bedf72de9fb2672d4193b3e5b725cdeab2ecde4d56074`.
The kernel implementation has not been edited; the named boot address produces
the same boot bytes as the former sed substitution.

Fresh checkout, runtime, clean/rebuild and final preservation acceptance are
pending. Work/evidence: `/tmp/qdos386-r7-canonical-dqnq2r1o/`. This candidate
checkpoint is not the final acceptance freeze.
