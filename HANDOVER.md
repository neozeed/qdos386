# Apollo R6 — A-only private alpha, FAT32 stopped

Canonical `/data/projects/qdos386/repository`, branch `r6-canonical`, is the safe A-only baseline. Accepted A source: `r6a-fat16-single-disk-accepted` (`1bd468e`); candidate archived source `05534f8`. Exact one-disk image SHA `3b3ceb41210c9726af2cd65c55225a5571f9df7193c96d722ce017506560396a`.

[Launch/use](reports/r6/README-ALPHA.md), [final handover](reports/r6/HANDOVER.md), [packaging receipt](reports/r6/PACKAGING.md), [tests](reports/r6/TEST-RESULTS.md), [Git graph](reports/r6/FINAL-GIT-GRAPH.txt). Private image/launchers: `../reference/qdos386/r6-single-disk-internal/alpha-candidate/`. Fresh source/image reproduction and one-drive GCC/HI pass.

FAT32 B1 failed the new RO close-callback contract and was stopped. Its source remains on `r6-single-disk` (`626c161`) with [counterexample](reports/r6/STAGE-B-INCOMPLETE.md) and private stage-b archives. No accepted B tag, no B2/B3, no public release/push/R7. All frozen R5/R4/R3/R2 assets, historical software and locked archival worktree remain intact. Next human decision is review/resume of B1 cleanup/callback repair.
