Recovered historical LP64 GCC 1.40 source

All 289 staged compiler source files and 13 generated inputs match the SHA256SUMS of the user-supplied R14B archive. Its host-bin/cc1 matches the accepted 1dd52eeb binary. Reverse application of the archive gcc-1.40.patch restores host64.h and the faulty expr.c. The archived scripts/r11l-host-cc1.sh documents source list and host flags. No source VCS for the original legacy toolchain is present; this is an explicit recovered-source checkpoint.

The older ORIGINAL.SHA256 validates 21 recovered files directly and 249 after restoring CRLF, including expr.c. 19 files do not match either uniform line-ending form; their raw pre-normalization identity is not claimed. They match the archived normalized staging source. GCC 13.3 used for the accepted executable is not installed; new builds use GCC 15.3 GNU89. Binary byte identity is not claimed for the new host executable.
