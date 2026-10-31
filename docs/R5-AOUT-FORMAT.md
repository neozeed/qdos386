# QDOS/386 R5 - a.out format audit (Phase 0)

This note records the executable format produced by the existing toolchain
(GCC 1.40 `cc1`, GAS 1.38 `as`, binutils 1.94-beta `i386-ld`, `a.out-i386`
backend). Nothing in the toolchain was changed. Every value below was read from
bytes of a real linked file (`apps/probe-layout/p2-layout.aout`).

## 1. Result in one paragraph

The output is an **OMAGIC (0407) a.out** file: a 32-byte header, text, data,
a symbol table and a string table. It has **no relocation records**
(`trsize = drsize = 0`). The code is fixed to the address it was linked at.
The header holds **no load address and no data or BSS address**. The loader
must therefore be told the link origin by the link script, and must derive the
data and BSS addresses from it. R5 uses an application-only link script with
origin `0x00200000`. The unchanged `i386-ld -N` produces the file.

## 2. Header (32 bytes, little-endian, eight 32-bit words)

| Byte offset | Field | Value in `p2-layout.aout` | Notes |
|---|---|---|---|
| 0x00 | `a_magic` | `0x00000107` (bytes `07 01 00 00`) | `0407` octal = OMAGIC |
| 0x04 | `a_text` | `0x00000060` (96) | text size, 16-byte multiple |
| 0x08 | `a_data` | `0x00000020` (32) | data size, includes 12 bytes of padding |
| 0x0C | `a_bss` | `0x00000020` (32) | BSS size, not stored in the file |
| 0x10 | `a_syms` | `0x00000078` (120) | 10 symbols x 12 bytes |
| 0x14 | `a_entry` | `0x00200000` | absolute address, the link origin |
| 0x18 | `a_trsize` | `0x00000000` | no text relocation records |
| 0x1C | `a_drsize` | `0x00000000` | no data relocation records |

Checked on the file: magic, `trsize = drsize = 0` and `entry = origin` all hold.
Every probe was linked with `-N`, and every probe gave `0407`. Other link flags
(for example without `-N`) were not tested, so no claim is made about them. The
R5 loader accepts only `0407` and rejects every other magic.

## 3. Byte-offset layout

```
file offset   content                         VA (origin 0x200000)
0x0000        header, 8 words (32 bytes)      -
0x0020        text   (0x60 bytes)             0x200000 .. 0x20005F
0x0080        data   (0x20 bytes)             0x200060 .. 0x20007F
0x00A0        symtab (0x78 = 10 entries)      -
0x0118        strtab (size word + names)      -
0x0163        end of file (363 bytes)         -
              BSS, not in file (0x20 bytes)   0x200080 .. 0x20009F
```

Derived invariants, all checked on the probe:

- `data_va = origin + a_text` (text is padded to 16 by the script).
- `bss_va = data_va + a_data`.
- `_bss_start = bss_va` and `_bss_end = bss_va + a_bss`.
- File offset of text = 32. Of data = 32 + a_text. Of symtab = 32 + a_text + a_data.

## 4. Symbol table

12-byte entries: `strx` (4), `type` (1), `other` (1), `desc` (2), `value` (4).

| Name | type | value |
|---|---|---|
| `gcc_compiled.` | 0x04 | 0x00200008 |
| `_start` | 0x05 | 0x00200000 |
| `_main` | 0x05 | 0x00200028 |
| `_table` | 0x07 | 0x00200060 |
| `_msg` | 0x07 | 0x0020006C |
| `_get` | 0x05 | 0x00200008 |
| `_zbss` | 0x09 | 0x00200080 |
| `_counter` | 0x09 | 0x00200094 |
| `_bss_start` | 0x09 | 0x00200080 |
| `_bss_end` | 0x09 | 0x002000A0 |

The string table starts at file offset `32 + a_text + a_data + a_syms`. Its
first word is its own size (83 here, which covers the size word and the names).
The loader does not need the symbol table. It is kept only for debugging.

## 5. Why the link origin is 0x200000

The stock script (`runtime/i386.ld`) places text at VA 0. That address is in
the kernel's low memory (BIOS data area, kernel image, heap), so it cannot be
used for an application. The unchanged linker accepts an application-only
script that sets the origin:

```
OUTPUT_FORMAT("a.out-i386")
OUTPUT_ARCH(i386)
SECTIONS {
 .text 0x200000 : { *(.text) . = ALIGN(16); }
 .data ALIGN(16) : { *(.data) . = ALIGN(16); }
 .bss ALIGN(16) : { _bss_start = .; *(.bss) *(COMMON) . = ALIGN(16); _bss_end = .; }
}
```

Without the explicit `ALIGN(16)` after `.data`, the linker placed BSS 4 bytes
past the data end (0x200078 instead of 0x200074). The layout is only
deterministic with the explicit alignment. The loader depends on this.

Linker facts observed:

- `i386-ld` does not emit relocations for this script, so a fixed-origin image is
  complete. Undefined symbols would make the link fail, and none remained.
- `.comm` symbols are placed in BSS (`COMMON`), so uninitialised globals are
  BSS and do not add file bytes.
- Initialised data includes alignment padding inside its declared size, so
  `a_data` is always a multiple of 16.

## 6. What the loader must check (from this audit)

1. Read exactly 32 header bytes. Reject if the file is shorter.
2. `a_magic == 0x107`. Reject any other value.
3. `a_trsize == 0` and `a_drsize == 0`. Otherwise reject as "relocation present".
4. `a_text`, `a_data`, `a_bss` are multiples of 16, and `a_entry == origin`
   (origin is the R5 link address, `0x200000`).
5. `a_entry` lies in `[origin, origin + a_text)`.
6. `32 + a_text + a_data + a_syms <= file_length` (no overflow). The symbol and
   string tables are not loaded.
7. Load `a_text` bytes from file offset 32 to `origin`, `a_data` bytes from file
   offset `32 + a_text` to `origin + a_text`. Zero `a_bss` bytes at
   `origin + a_text + a_data`.
8. The loaded image must fit in the application region (see the R5 ABI note).

## 7. Second program (phase 0 step 4)

`apps/probe-layout/p2.c` has initialised `int table[3]`, an initialised
character array `msg`, uninitialised `zbss[5]` and `counter`, and calls through
a function. The compiled code used absolute addresses (`movl _table(,%eax,4)`),
confirming there is no PC-relative relocation to fix. The binary is fully linked
at its origin. Its text is 96 bytes and data is 32 bytes. BSS is 32 bytes.

## 8. Not verified yet

- The other probe (`p1`, origin 0x200000 from an earlier script) gave text 48,
  data 4, bss 24. It used the older script without the explicit alignment. It
  is kept only as history, and it is not used in R5.
- Exact behaviour of the unchanged `-O` driver on large programs. Only small
  probes were built.
- Whether gcc emits helper calls (for example `___fixdfsi`) for other code. The
  probe uses integers only. Any undefined helper would fail the link.
