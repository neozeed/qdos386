#!/bin/sh
# KCTX-FIX2: derive a PUBLIC-SAFE source tree from a committed QDOS/386 tree (the full tree carries two restricted inputs).
# Left out:  apps/superbasic/            SuperBASIC - private research, not freely redistributable
#            third_party/infocom/orig/   Task Force '87 interpreter source - no licence in the archive
#            third_party/infocom/r11g-platform.patch (a patch of that source)
# Everything else is kept with its notices (GPL QLTerm third_party/qlterm-c-r1, optional GPL Minerva third_party/minerva,
# Hack 1.0.3 third_party/hack103-ql, phoon, the R11 fixtures).  What no longer builds without them: sdk/examples
# BASIC.aout / BASTEST.aout, scripts/r11g-build.sh (INFOCOM.AOU) and the SuperBASIC host tests.  The kernels (`make`)
# do not need them (checked by the package verification).  PUBLIC-SOURCE.md lists what was removed.
#   usage: sh scripts/qdos386-public-source.sh OUTDIR [COMMIT]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
O=${1:?}; C=${2:-HEAD}; mkdir -p "$O"
git archive "$C" | tar -x -C "$O"
rm -rf "$O/apps/superbasic" "$O/third_party/infocom/orig" "$O/third_party/infocom/r11g-platform.patch"
cat > "$O/PUBLIC-SOURCE.md" <<EOP
# Public-safe source tree (from $(git rev-parse "$C"))

Removed: \`apps/superbasic/\` (SuperBASIC: private research, not freely redistributable), \`third_party/infocom/orig/\`
and \`third_party/infocom/r11g-platform.patch\` (Task Force '87 interpreter: no licence granted; see
\`third_party/infocom/PROVENANCE.md\`, kept). No story file is part of any QDOS/386 tree.
Not buildable here as a result: \`sdk/examples\` BASIC.aout / BASTEST.aout, \`scripts/r11g-build.sh\`, the SuperBASIC host
tests. The kernels (\`make\`) build without them. The public-safe system volume is made from the FULL tree by
\`scripts/qdos386-public-vol.sh\` (which removes the restricted programs from the image).
EOP
echo "public-safe source tree in $O"
