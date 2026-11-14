#!/bin/sh
# Compare apps/superbasic/src with the SB4 distribution checksums (SHA256SUMS.sb4).
# Expected: every core file identical except sb_state.h (R12: one #ifndef guard around SB_MAX_CALL_DEPTH; R12B: the host
# extension name table SbExtName and two SbState fields), sb_interp.[ch] and sb_expr.c (R12B: the extension-name lookups,
# sb_new, sb_in_direct_mode).  The full difference is qdos386-core-changes.patch;
# sb_console.[ch] and sb_main.c are not part of this port (replaced by qdos/).
cd "$(dirname "$0")" || exit 2
bad=0
grep ' src/' SHA256SUMS.sb4 | while read -r sum path; do
  f=${path#src/}
  if [ ! -f "src/$f" ]; then echo "not ported: $f"; continue; fi
  got=$(sha256sum "src/$f" | cut -d' ' -f1)
  if [ "$got" = "$sum" ]; then echo "identical: $f"; else echo "CHANGED:   $f"; fi
done
