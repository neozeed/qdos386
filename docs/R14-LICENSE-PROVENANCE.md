# R14 licence and provenance

Each asset is listed with what its own files state. Nothing is inferred from one asset to another.

## 1. QLTerm C R1 (`third_party/qlterm-c-r1/orig/`)

* Supplied by the user as `QLTERM-C-R1-FULL-SOURCE.zip`, 162,576 bytes, SHA-256
  `fb7e46c112c494fe756eb20f9af8672ec52b0e513b627443a6416b625c3edd5b`. The `qltc/` tree is imported unchanged
  (its own `SHA256SUMS`, 53 entries, all verify), including the 68000 executable `qlterm` built by its author
  (never executed here), the original assembly in `reference/`, the QLBox-based tests and the validation logs.
* Licence as stated in the archive: **GPL version 3 or later**. `README.md`: "GPL version 3 or later, derived from
  Jan Bredenbeek's QLTerm (1987–2026). Original notices and GPL text are included. The C translation and test scripts
  in this package are distributed under the same licence." `LICENSE` and `reference/LICENSE` hold the GPL-3.0 text;
  `reference/MAIN.ASM` carries the author's notice ("Licenced under GPL v3 Licence (2019)", the GPL-3-or-later
  grant, and the upstream https://github.com/janbredenbeek/QLTerm). Each C file carries "GPL-3.0-or-later".
* Consequences for R14: the QDOS/386 port (the narrow source patch, the compatibility layer that is linked into the
  QLTerm program, the build scripts and the QLTerm tests) is distributed under **GPL-3.0-or-later** with the original
  notices kept. The QDOS/386 kernel and SDK are not derived from QLTerm and are linked with it only as an operating
  system and its C runtime.
* The original project is not modified in place: R14's changes are a patch file applied to a build copy.

## 2. Other R14 material

* Serial driver, libqdos serial calls, the QL compatibility layer, host peers and tests: written for R14 in this
  repository (same terms as the rest of QDOS/386; the compatibility layer is additionally offered under the GPL as
  part of the QLTerm build).
* XMODEM/YMODEM testing uses **lrzsz 0.12.21** (Ubuntu package `lrzsz 0.12.21-11build1`, GPL-2.0-or-later,
  https://ohse.de/uwe/software/lrzsz.html: `sz`, `rx`, `rb`) as the independent implementation on the host. It is a
  host tool installed from the distribution, driven by `tests/r14/qltpeer.py`; it is **not** shipped in the package.
* The QXF1 transfer probe (`sdk/examples/r14/xfer`, `tests/r14/serpeer.py`) is written for R14 and is not an
  implementation of any published protocol.

## 3. Restricted material carried by example volumes

* SuperBASIC (`/BIN/BASIC.AOU`, `apps/superbasic/`): **private research, not freely redistributable** (see
  `apps/superbasic/docs/PROVENANCE-LICENSING.md`). R14 test volumes that contain it must not be published publicly;
  the R14 package makes no redistribution claim for it.
* The Infocom interpreter and story file rules of R11G are unchanged (`third_party/infocom/PROVENANCE.md`).
