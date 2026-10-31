# R11M Stage 4: VFAT long file names on FAT16 - what is implemented, what is not

Kernels built with `-DQDOS_VFAT` (all R11M kernels; it needs `-DQDOS_FS_WRITE`). Kernels without the macro, including every frozen
default image, contain none of this code. The legacy `WIN1_NAME` interface is unchanged.

## 1. Encoding policy (stated, not hidden)

* **Long names are an ASCII subset.** A name the guest can create or look up by its long name is 1..**63** characters of printable
  ASCII (0x20..0x7E), none of `\ / : * ? " < > |`, not starting with a space or a dot, not ending in a space or a dot. The limit 63
  follows the 64-character path limit of the native path ABI (`docs/R11M-FAT-PATH-ABI.md`). Anything else is refused with
  `FAT_ERR_NAME` (-46, "invalid name"): `*`, `?`, control characters, bytes above 0x7E, a leading dot (`.hidden`), 64 or more
  characters. There is no UTF-8 or code-page interpretation of names; a byte above 0x7E is never accepted.
* **Reading is wider than writing.** A long-name chain on the volume (made by Windows, Linux `vfat`, mtools) is decoded as UTF-16 units.
  If every unit is in the subset above and the length is 1..63, the entry is *representable*: it is listed by its long name and found by
  it. Otherwise (non-ASCII, over 63 units, a unit that is not allowed) the chain is *valid but unrepresentable*: the entry is listed by
  its 8.3 alias with the flag `QD_DIRL_ALIASONLY` and is found only by that alias. Deleting such an entry removes its chain too.
* **Entries with no usable name are invisible.** If the 8.3 alias itself contains a byte above 0x7E (mtools writes `HÉLLOW~1.TXT` in
  code page 850 for "héllo wörld.txt") the entry can neither be listed nor opened (it is counted in `fat16_dir_skipped`, as an invalid
  name always was). It stays intact on the volume, and a directory that holds one cannot be removed (`NOTEMPTY`).
* **Case.** Lookup is case-insensitive over ASCII letters. A name that is already a valid 8.3 name once upper-cased is stored as a
  short entry only, **in upper case** (typing `readme.txt` creates `README.TXT`, with no long-name entry; the typed case is lost). A name
  that needs a long entry keeps the case it was typed in. The NT case-flag byte (offset 12) is written as 0 and ignored when
  reading, so a host file `lower.txt` stored as `LOWER.TXT` with case flags is found by `lower.txt` and listed as `LOWER.TXT`.
* **Short-name characters.** In this kernel a short component may contain `A-Z 0-9 ! # $ % & ' ( ) - @ ^ _ ` { } ~` (and, in native
  paths, lower-case letters, folded). That is what lets `SUPERB~1.C` be typed. The legacy `WIN1_` form keeps its old rule (upper-case
  letters, digits, `_`), and **underscores are never reinterpreted**.

## 2. Names and aliases

A name that needs a long entry gets an alias `BASE~N.EXT`:

* `EXT` is the first three allowed characters after the last dot; `BASE` is the part before the last dot with blanks and dots removed,
  upper-cased, any other character outside the short set replaced by `_`, truncated so that `BASE~N` is at most 8 characters (6 for
  N<10, 5 for N<100, 4 for N<1000). An empty base becomes `_`.
* `N` is the smallest number from 1 that makes the alias unused **among the live short entries of the same directory**; beyond 999 the
  create fails with `FAT_ERR_EXIST` (-83).
* Examples verified on the guest and by mtools: `SuperBasic_Interpreter.c` -> `SUPERB~1.C`; `Hello World.txt` -> `HELLOW~1.TXT`; twelve
  names `Alpha Beta Gamma 1..12.txt` -> `ALPHAB~1..9.TXT`, `ALPHA~10..12.TXT`; `a+b.txt` -> `A_B~1.TXT`; `ABC.DEFG` -> `ABC~1.DEF`;
  `this.is.a.test.c` -> `THISIS~1.C`.
* The generated aliases are deterministic but are **not** the aliases Windows would choose beyond `~4` (Windows switches to a hash
  form). Both are valid FAT; only this implementation's own numbering is promised.
* Both names address the same directory entry. The open-file registry is keyed by that entry, so a file open under one name is
  busy under the other.

## 3. Lookup, listing, rename, delete

* **Lookup:** the short name first (when the component is a valid short name), then a case-insensitive scan of the long names. `SRC/SUPERBASIC_INTERPRETER.C`,
  `src/superbasic_interpreter.c` and `SRC/SUPERB~1.C` open the same file. Directory components may be long names too.
* **Listing:** the 16-byte `WIN1_` records are unchanged and show the **alias** (so older programs and `opendir` in the C library see
  names they can pass back). `CHAN_CTL_LISTLONG` (`CH_CTL` op 5) on a directory channel switches it to 96-byte records:
  name[64] (the long name or the alias), alias[16], size, attribute, flags (1 long name, 2 alias only), reserved. `qd_dir_open_long` /
  `qd_dir_next_long` wrap it (`QD_ERR_NOTSUP` -15 on a kernel without VFAT). The shell `DIR` uses it and falls back to the short listing.
* **Rename:** same directory and the old entry has no long-name chain and the new name is a short name: the 11 bytes are rewritten in
  place. Every other case writes the new entries first (short entry, then its chain), then deletes the old ones. A directory still
  cannot change parent (`FAT_ERR_XDIR`). An existing target of either name gives `FAT_ERR_EXIST`.
* **Delete / rmdir:** the entry's *trusted* chain is marked deleted first (highest ordinal first), then the short entry, then the
  clusters are freed. A directory whose only entries are long-name entries counts as empty.
* **getcwd** returns long names where the directory has one that is representable. The result can be longer than 64 characters and then
  cannot be passed back as one path; use relative paths (`..`, `NAME`) from such a directory.

* **Shell:** a word containing blanks is written in double quotes (`COPY LONG.TXT "My Report.txt"`, `CD "My Documents"`, `RUN "Hello Program.aou"`);
  the quotes are removed before the name reaches the kernel. `DIR` prints `ALIAS  SIZE|<DIR>  LongName`. The C library maps a bare
  program file name that is not a legal legacy 8.3 name to the native form `WIN1:name`, so `fopen("Long Name.txt", ...)` works from the root.

## 4. Which chains are trusted

A chain is used only when all of these hold: it starts with an entry carrying ordinal|0x40 (1..20), the following entries carry
ordinals counting down to 1 with no gaps, every entry has attribute 0x0F, type byte 0, first-cluster field 0 and the same checksum,
the characters before the terminator `0x0000` are present and all units after it are `0xFFFF`, the terminator does not sit at the
start of the first entry, and the checksum equals the checksum of the **next** short entry's 11 name bytes. A deleted entry, a volume
label or another short entry ends a chain. Otherwise the long name is ignored:

* the short entry is seen with no long name (its alias works, it lists as its alias);
* the long-name entries stay on disk **untouched** (not deleted, not repaired, not reused); `r11m-fatwalk.py --vfat` reports them as
  errors, which is intended;
* an orphan chain (its short entry was deleted by a tool that left the chain) is just a run of occupied slots.

## 5. Order of writes, failures

Write-through as in R11L, no crash-atomicity promise: **create** writes the short entry, then the chain (a cut leaves a short entry
under its alias, or a chain that is not trusted; never a wrong name); **delete** chain, then short entry, then FAT; **rename** new
entries before old ones are deleted (a cut leaves two names for one chain, reported by the checker, nothing lost). A directory
that cannot grow (`FAT_ERR_DIRFULL`: the root's fixed size, 64K entries, or no free cluster) fails before anything is written; a
create needs a contiguous run of `1 + ceil(len/13)` free slots (2 to 6), so a root with scattered holes can refuse a long name that
a short name would fit.

## 6. Not implemented

Non-ASCII long names; Unicode case folding; case-preserving short names; the NT case flags; FAT32; aliases beyond `~999`; names over
63 characters (they are read as alias-only); changing the alias of an existing file; repair of damaged chains; Windows-compatible
alias numbering past `~4`; long names in the legacy `WIN1_NAME` form (refused with `FAT_ERR_NAME`); unlisted entries whose alias holds
non-ASCII bytes.

## 7. Evidence

| Evidence | Class |
|---|---|
| `tests/host/r11m/test_vfat.c`: 343 checks on two volume shapes (gate scenario, aliases, 12 and 999 collisions, rejected names, rename matrix, long directories, getcwd, delete and re-create, 120-name directory growth, remount); 24 on a volume made with mtools (host-created long, mixed-case, non-ASCII, CJK and 100-character names); 73 on malformed chains made by editing sectors (bad checksum, missing 0x40, order, type, cluster field, orphan, renamed short entry, non-ASCII unit, damaged padding, 78-unit chain); 9 on a full root. Strict C89, `-Werror` | host build / independent comparison |
| `fsck.fat -n`, `tools/r11m-fatwalk.py --strict --vfat` (recursive, decodes chains and checks ordinals, checksums, padding) and mtools (`mdir`, `mcopy` by long name and by alias, content compared) on the final images of the above | independent comparison |
| `sdk/examples/r11m/vfattest` (`VFATT.AOU`): 68 guest checks through libc and libqdos | guest executed (i386VM, Tiny386, QEMU) |
| `scripts/r11m-vfat-gate.sh vm\|pc\|qemu` (`GATE vm PASS`, `GATE pc PASS`, `GATE qemu PASS`; also `RUN` of a program by quoted long name and by alias): guest creates `/SRC/SUPERBASIC_INTERPRETER.C` and other long names, **reboot**, read by long name and by alias, renames and copies, **reboot**, deletes; `fsck.fat`, fatwalk and mtools judge each image | guest executed + independent comparison |
