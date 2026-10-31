# R1 validation results

Clean build: `make clean`, then `make -j2 check` on Linux x86-64.
The bundled host64 GAS 1.38 built successfully. All twelve assembly modules
assembled as i386 OMAGIC relocatable objects. All external symbols resolve
within the translated set. Combined text plus data is 3,696 bytes (before any
final executable linkage). Public definitions: 38 callable C entries, one
internal sign_add helper, one ship_fit data symbol.

All eleven suites passed with zero failures; see GAS138-TEST.log. Ten original
C test drivers and their fixtures are unchanged. Extra tests cover the original
packed DORND fixtures, chart checks extracted from fidelity_test.c, and escape
state preservation. Reference-file byte comparisons against the attached RC6
archive all passed.

The executed gameplay machine code comes from the actual GAS 1.38 objects,
via the test-only relocation adapter. This is emulated i386 execution, not a
QDOS386 boot. No playable game, full assembly translation, complete RC6 parity,
or video/system integration has been verified. No placeholder game functions
are linked into these tests.

prebuilt/aout contains the twelve objects from this passing clean build.
The main deliverable is the editable assembly and reproducible build/test setup.
