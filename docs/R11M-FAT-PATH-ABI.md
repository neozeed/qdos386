# R11M FAT16 subdirectories and the native path ABI

Applies to kernels built with `-DQDOS_FS_WRITE` (the R11L writable-volume option). Kernels without it are unchanged and keep the
read-only root-only behaviour.

## 1. Path convention

| Form | Meaning |
|---|---|
| `WIN1:/SRC/HELLO.C` | native absolute path |
| `WIN1:SRC/HELLO.C` | native path relative to the calling job's current directory |
| `WIN1_HELLO.AOU` | legacy flat form: the root file `HELLO.AOU` (strict upper-case 8.3) |
| `WIN1_` | the volume root (a listing channel, files only, as in R7..R11L) |
| `WIN1_A/B.C` | root-anchored native path `/A/B.C` (a legacy name never contains `/`) |

Rules, all implemented in `fat16_resolve` and `win1_norm` (kernel), not in a shell alias table:

* Separator `/`. A leading `/` is the volume root. `.` is the directory itself, `..` its parent; **`..` at the root is the root**, so a
  path cannot leave the volume. Empty components (`//`) are skipped. A trailing `/` requires the target to be a directory (`SRC/HELLO.C/` gives `NOTDIR`; `mkdir`, `remove` and `rename` refuse a trailing slash on a name they would create or delete as a file).
* A component is an 8.3 name (1..8 base characters, optional `.` and 1..3 extension characters; upper-case letters, digits and `_` only;
  lower-case letters are accepted in native paths and folded). Lookup is case-insensitive for native paths; created names are stored upper case.
* At most `FAT_PATH_MAX` (64) characters per path and `FAT_PATH_DEPTH` (16) components; longer paths fail with `FAT_ERR_DEPTH` before any
  lookup. The channel name buffer is `CHAN_NAME_MAX` 72 characters (prefix included).
* **Underscores are never reinterpreted.** `WIN1_ZORK1_Z3` is the root file `ZORK1_Z3`; `WIN1_SRC_X.TXT` is the root file `SRC_X.TXT`
  and never `/SRC/X.TXT`. A legacy name that is not a valid upper-case 8.3 name is refused (`FAT_ERR_NAME`; one with more than 12 characters after the prefix, i.e. longer than any 8.3 name, keeps its R11E answer `CHAN_ERR_RANGE` -14, which `QDOSAPI.AOU` checks), and a legacy name never
  addresses a directory (`FAT_ERR_NOFILE`).
* The C library (`qd_fsname`) maps a program's file name to a channel name: `CON` stays; `WIN1_...` and `WIN1:...` are passed through
  (legacy flat names folded to upper case); a name containing `/`, and any name while the job's current directory is not the root, becomes
  `WIN1:` + name; a bare name in the root stays `WIN1_NAME` exactly as before R11M. The shell prefixes `WIN1:` to its path arguments and
  passes arguments that already start with `WIN1_` or `WIN1:` unchanged.

## 2. Current directory policy

* Per job. A new job starts in the directory of the job that created it (`job_alloc_sized` copies it). `CHDIR` changes only the caller's.
* The shell is a job, so `CD` changes the directory its `RUN` children inherit. A child's `CHDIR` does not affect the shell.
* A directory that is the current directory of any job cannot be removed or moved (`FAT_ERR_BUSY`).
* A job that ends releases its entry. Directory cluster numbers are the stored form; the absolute path is recomputed on demand by walking
  `..` (`FS_GETCWD`), so a renamed ancestor is followed correctly.

## 3. Traps

`int 0x30` convention as before (eax = trap, ebx, ecx, edx = arguments).

| Trap | Use |
|---|---|
| `0x2A CH_OPENX` (name, flags) | open by channel name. Flags: `READ 1`, `WRITE 2`, `CREATE 4`, `TRUNC 8`, `APPEND 0x10`, `EXCL 0x20`, **`DIRS 0x40`**, **`FILE 0x80`**. |
| `0x2B CH_CTL` (ch, op, arg) | flush, truncate, size, tell (files only; a directory channel supports flush as a no-op). |
| `0x2C FS_OP` (op, a, b) | see below. |

`FS_OP` operations (a, b are user addresses unless noted; every pointer is range-checked):

| op | name | arguments | result |
|---|---|---|---|
| 1 | REMOVE | a = file path | 0. A directory gives `ISDIR`; an open file `BUSY`. |
| 2 | RENAME | a = old, b = new | 0. Files may move between directories; a directory only within its parent (`XDIR`). Both names must be on the same driver. An existing target gives `EXIST` (never replaced). |
| 3,4,5 | FREE, CLUSTER, TOTAL | any WIN1 name | free clusters, bytes per cluster, data clusters |
| 6 | MKDIR | a = path | 0. Allocates one zeroed cluster holding `.` and `..`. |
| 7 | RMDIR | a = path | 0. Empty directories only (`NOTEMPTY`); not `/`, `.`, `..` (`INVAL`); not a job's cwd (`BUSY`); a file gives `NOTDIR`. |
| 8 | CHDIR | a = path | 0. A file gives `NOTDIR`. |
| 9 | GETCWD | a = buffer, b = size (1..256, raw value) | length of the absolute path (`/`, `/SRC`), or `DEPTH` if the buffer is too small. |
| 10 | STAT | a = path, b = 8-byte buffer | 0 file / 1 directory; buffer = {size u32 LE, attribute u32 LE}. |

Open behaviour for `CH_OPENX`:

* A directory opened `READ` is a **listing channel**: 16-byte records `{name[12], size u32}`; `.` and `..` are not listed; files only unless
  `DIRS` is set, in which case a subdirectory has size `0xFFFFFFFF`. `DIRS` on a file gives `NOTDIR`. `FILE` makes a directory target
  fail with `ISDIR`. Opening a directory for writing gives `ISDIR`. `libc` `fopen`/`open` always pass `FILE` for native names.
* The conflict check (open-file registry keyed by the directory entry's sector and offset) happens before anything is created or
  truncated: any number of readers or one writer per file, across all directories.

## 4. Errors

R11L codes are unchanged. New: `-87 NOTDIR`, `-88 ISDIR`, `-89 NOTEMPTY`, `-90 DEPTH`, `-91 INVAL`, `-92 XDIR`. Existing codes used
by the path layer: `-44 NOFILE`, `-46 NAME`, `-43 CHAIN`, `-81 FULL`, `-82 DIRFULL`, `-83 EXIST`, `-84 BUSY`, `-80 WPROT`. `errno` mapping
(libc): `NOTDIR->ENOTDIR`, `ISDIR->EISDIR`, `NOTEMPTY->ENOTEMPTY`, `DEPTH->ENAMETOOLONG`, `INVAL->EINVAL`, `XDIR->EXDEV`, `EXIST->EEXIST`,
`BUSY->EBUSY`, `NOFILE->ENOENT`, `FULL/DIRFULL->ENOSPC`.

## 5. On-disk consistency

* A new directory is one cluster: entry 0 `.` (its own first cluster), entry 1 `..` (the parent's first cluster, **0 for the root**), the rest
  zero. Directory entries record no size (0). Directories grow a cluster at a time when full; the root has a fixed size
  (`FAT_ERR_DIRFULL` when it has no free run).
* `..` is validated when it is followed (`dir_parent`): a damaged or looping `..` gives `FAT_ERR_CHAIN` rather than an endless walk. A cycle
  in a directory's own cluster chain is detected by a bounded walk.
* Flush order (write-through): data clusters, then the FAT (copy 1, then copy 2), then the directory entry. For `MKDIR`: the new cluster is
  zeroed and filled first, then the FAT, then the parent's entry. For `RMDIR`/`REMOVE`: the entry is marked deleted first, then the chain
  is freed in both FAT copies. **There is no crash-atomicity promise**: a power loss between steps can leave orphan clusters (reported as
  warnings by `tools/r11m-fatwalk.py`) or, for MKDIR, an unreferenced directory cluster.
* Full disk is `FAT_ERR_FULL`; a full fixed root or a full directory that cannot grow is `FAT_ERR_DIRFULL`; failures leave the volume
  consistent (a failed create releases what it allocated).

## 6. Shell commands (R11M shell)

`PWD`, `CD [path]`, `MD path`, `RD path`, `DIR [path]` (names, sizes, `<DIR>`, counts), `TYPE path`, `DEL path`, `REN old new`,
`COPY src dst [/Y]`, `RUN path [args]`. The prompt shows the current directory (`/SRC> `).

* `COPY`: refuses an existing destination (`destination exists (nothing copied; COPY ... /Y replaces it)`) unless `/Y`; a destination that is
  a directory receives the file under the source's name; directories are never copied. Without `/Y` a failed copy removes the partial
  destination; with `/Y` the destination was truncated first, so a failed copy leaves an incomplete file and says so.
* `RUN name`: the name is resolved relative to the current directory, then, for a bare name only, in `/BIN`. `WIN1_NAME` and `WIN1:path`
  forms are accepted. Programs themselves are ordinary slot-1 a.out images: the directory they are found in is irrelevant to the loader.

## 7. Verification

| Evidence | Class |
|---|---|
| `tests/host/r11m/test_fatdir.c`: 124 checks on three volume shapes (nested directories, empty and nonempty removal, relative paths, `..`, duplicates, case-insensitive lookup, full root, full chain, out of space, invalid and over-long paths, damaged `..`, looped directory), judged by `tools/r11m-fatwalk.py --strict` and `fsck.fat -n`, strict C89 with `-Werror` | host build / independent comparison |
| `sdk/examples/r11m/pathtest` (`PATHT.AOU`): 68 guest checks through libc and libqdos (fopen, rename, remove, mkdir, rmdir, chdir, getcwd, stat, listings, legacy names, bad paths) | guest executed (i386VM, Tiny386, QEMU) |
| `tests/r11m/seq-s1a.txt` then `seq-s1b.txt`: create, copy, nested directories, rename, removal errors, **reboot**, read back by absolute and relative paths, `RUN` of legacy and native names | guest executed on three emulators; final images checked by `fsck.fat -n` and `tools/r11m-fatwalk.py --strict` |
