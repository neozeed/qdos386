# Provenance: Task Force '87 interpreter and Zork I story (R11G)

Two separate assets, with separate licence status. Do not infer one from the other.

## 1. Interpreter (`orig/`, `r11g-platform.patch`)

- Source archive supplied by the user: `75f9e61b-infocom.zip`, SHA-256
  `21ae323dd01b0ad4f30a6686a7cba40dc9135b1e7f1aec43c3c23f2810eae7ca`, 123,034 bytes.
- Contents: 16 C modules and `infocom.h` (CRLF line endings), `makefile`, and pre-built objects,
  a binary and a QL boot script that the port does NOT use. The pre-built objects and binary were
  never executed.
- Copyright lines: "(C)opyright 1987 InfoTaskforce." (16 files; `object.c` reads "IntTaskforce",
  kept as supplied). `infocom.h` is dated 23 June 1987.
- Licence status: NO licence statement is in the archive. No licence is granted by this package for
  the interpreter. It is used here for private compatibility testing. Redistribution of
  `third_party/infocom/orig/` needs the rights holder's permission; it is included in the R11G
  package only as the build input, and that is a decision for the package owner.
- `orig/` is the pristine tree (CRLF, byte-identical to the archive). The QDOS/386 change is
  `r11g-platform.patch` only, applied to LF build copies by `scripts/r11g-build.sh`.

## 2. Story file (`zork1_z3`, not in this repository)

- File name in the archive: `zork1_z3`, 86,838 bytes, SHA-256
  `37084966477dff679282de42974b2077156b1bd68fad92a65d4ea94d8eb64d79`.
- Z-code version 3, release 119, serial 880429 (read from the header).
- Licence status: NOT verified for this file. Press reports (November 2025) say Microsoft released
  the Zork trilogy under the MIT licence. That statement has not been matched to this hash or to an
  official release file, and the archive contains no licence text. The story is therefore EXCLUDED
  from the R11G package and from the R11G volume distributed with it. `scripts/r11g-volume.sh`
  builds the volume from a supplied copy and refuses any file with a different hash.
- The MIT statement is not treated as covering the interpreter.
