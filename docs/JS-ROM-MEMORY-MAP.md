# JS ROM Memory Services: R3 Mapping (Phase 0, closed)

Source: `x/jsrom/js-rom-gas-r2/src/system_asm.s` (GAS port of the JS ROM).
Every claim below cites a line in that file. Items are classified as:

- **implemented**: R3 provides the behaviour, verified against source.
- **approximated**: R3 provides a narrower or different behaviour, stated below.
- **deferred**: real JS ROM behaviour that R3 does not provide.
- **unresolved**: not established from source; kept outside the R3 ABI.

Three columns are kept separate in §5: historical semantics, R3 native
implementation, and deliberately deferred compatibility. R3 is not binary- or
ABI-compatible with QDOS, and sharing a name does not make a service compatible.

## 1. Audit items (exit criterion of the continuation plan)

### 1.1 `l0305e`: vector entry and naming (resolved)

The source uses three labels for the same entry:

| Location | Text | Meaning |
|---|---|---|
| line 174 (vector table, `ooc0+4`) | `.word l0305e \| * MM.RECHP` | vector name |
| lines 920, 1569, 1670, 1698, 2037, 2790, 4207, 5130 | `jsr/jmp l0305e \| * MT.RECHP` | call-site name |
| line 4471 (above `l0305e`) | `MT.FREE find largest free space` | **misplaced comment** |

The body of `l0305e` (lines 4472–4486) releases a block. It is not a
largest-free query. The comment at 4471 describes `l030e0` (§1.3), which is
the largest-free routine (`moveq #1,d0` then `l03134`, lines 4523–4538).

Calling convention, verified from the call sites:

- **Input**: A0 = block base. The release size is the longword at `(A0)`
  (`move.l (a0),d1`, line 4472).
- **Output**: D0 = 0 (`moveq #0,d0`, line 4489). The caller's own D0 is
  preserved around the call where it matters (lines 1670, 5130).
- **Block prefix**: the job-owned release at `tr1d19` (lines 920–921) uses
  `lea -0x0010(a0),a0` before the call, so a job-owned heap block has a
  16-byte header in front of the job record. The size longword is the first
  longword of that header.

Classification: **approximated**. The name is MT.RECHP. The release
behaviour is implemented in R3 as `mem_free`. The return to the transient gap
is deferred.

### 1.2 `l031e0`: job-header and USP fix-up (resolved, not applicable)

Callers (verified):

- `l031b8` (line 4631, `bsr.s l031e0`), reached from `tr1d16` (line 951,
  allocate BASIC program area) and from the transient reserve path `l0301c`
  (line 4439).
- `l031c8` (line 4642, `bsr.s l031e0`), reached from `tr1d17` (line 960,
  release BASIC program area) and from the transient release path (line 4502).

What `l031e0` does (lines 4653–4670):

1. Sets SV.JBBAS (`0x68(a6)`) to the new base A0 (`move.l a0,(a3)`).
2. Subtracts D1 from two job-header longwords at offsets `0x58` and `0x5c`
   of the moved header (lines 4657–4658).
3. If SV.JBPNT (`0x64(a6)`) equals the old SV.JBBAS, it subtracts D1 from
   `0x8(a5)` and from USP (lines 4664–4667). This adjusts the BASIC job's
   stack and A5 base.

Why it does not apply to R3: `l031e0` runs only when BASIC program or
transient memory is moved. The moved data includes the BASIC job's header and
USP. R3 never moves memory. Its job stacks are allocated once, from a fixed
arena, and are not in the BASIC area. Each R3 job's saved stack pointer is a
plain 32-bit value that is never rewritten.

Classification: **deferred**. Relocation and pointer fix-up are not
implemented in R3.

### 1.3 `l030e0`: largest-free query (unresolved callers)

- `l030e0` is labelled `xltr1d06` (line 4524). It is reached only through
  the trap-1 dispatch word table entry `o7e: .word xltr1d06-l00460` (line
  574). No `jsr`, `bsr`, or `jmp` to `l030e0` exists in the file (grep).
- Behaviour (lines 4525–4538): D0 = 1, D1 = 0, call `l03134` on SV.TRNFR
  (largest-block mode), compare the largest block with SV.BASIC - 0x200 -
  SV.FREE, return the smaller in D1, and tail-jump to `l003a4`.
- Its caller is the trap dispatcher. The file does not show what a user
  program does with the result.

Classification: **unresolved**. Its function is known. Its trap code is
`0x1d06` by the naming convention, and that is not proven from a dispatch
decode. It is kept outside the R3 ABI.

### 1.4 `SV.CHPFR` sentinel (resolved from two consistent call sites)

Sentinel layout, derived from `l03134` (lines 4577–4590) and `l03162`
(lines 4586–4597):

- The list head is a node whose address is **A6+4 (SV.CHEAP, 0x04)**. Its
  size longword is not used as a size.
- The first link is the longword at **A6+8 (SV.CHPFR, 0x08)**. It is a
  **relative** offset from A6+4. Zero means the list is empty.
- Each free node is `size` at offset 0 and `relative next link` at offset 4.
  Zero terminates.
- `l02fae` (line 4393) relies on this: if the predecessor found by the search
  is the head (`cmpa.l a1,a2` with `a2 = A6+4`), it does not clear `8(a1)`.

Evidence: `l02ffa`/`l02fd6` pass `lea 0x0008(a6),a1` (SV.CHPFR) to `l0315e`,
which subtracts 4 before `l03162`. This gives A6+4 as the head, matching
`l03134`'s own `subq.w #4,a0`.

Classification: **implemented** in R3 as an explicit head sentinel with
relative links, so the layout is reproduced in spirit but not in bit-exact
form.

## 2. Entry points

| Vector label | Body label | Historical name | Role |
|---|---|---|---|
| `ooc0` `.word l02fae` | `l02fae` (4389) | MM.ALCHP | Allocate from common heap, zero-filled, with transient fallback |
| `ooc0+4` `.word l0305e` | `l0305e` (4472) | MM.RECHP / MT.RECHP | Release a block (size in first longword) |
| `od8` `.word l03104` | `l03104` (4538) | MM.ALLOC | Allocate from common heap, no zero-fill, 8-byte rounding |
| `od8+4` `.word l03162` | `l03162` (4586) | MM.LINKFR | Link an external free area into a list |

Helpers: `l03106`/`l03134` (search and allocate), `l0315e` (clear link and
link free space), `l0318c` (coalesce), `l0324e` (grow heap into transient
gap), `l0323a` (shrink gap for BASIC), `l03276`/`l03282` (SV.BASIC and
SV.FREE adjust), `l0320c`/`l03212` (512-byte rounding).

## 3. System variables (offsets from A6)

| Offset | Name | Use (verified) |
|---|---|---|
| 0x04 | SV.CHEAP | Head sentinel address of the common free list (§1.4) |
| 0x08 | SV.CHPFR | Relative link to the first common free node |
| 0x0C | SV.FREE | Top of common heap; bottom of the transient gap |
| 0x10 | SV.BASIC | Bottom of BASIC area; top of the transient gap |
| 0x14 | SV.TRNSP | Transient-program space pointer |
| 0x18 | SV.TRNFR | Head of the transient-program free list |
| 0x58 | SV.BTBAS | Base of the 512-byte block map |
| 0x64 | SV.JBPNT | Current job header pointer |
| 0x68 | SV.JBBAS | Job-header base (rewritten by `l031e0`) |

## 4. Register, flag and error conventions (verified)

| Service | Inputs | Outputs | Errors |
|---|---|---|---|
| MM.ALCHP (`l02fae`) | D1 = bytes | A0 = block base, D1 = rounded size | Z=1 success; Z=0 with D0 = -3 |
| MM.RECHP (`l0305e`) | A0 = block base | D0 = 0 | none |
| MM.ALLOC (`l03104`) | D1 = bytes; A0 = list head set by the caller | A0 = block base; D1 = rounded size | D0 = -3. CCR is not a success indicator (last flag-setting instruction is a plain move) |
| MM.LINKFR (`l03162`) | A0 = block, D1 = size, A1 = list head | links and coalesces | none |

Rounding: `l02fae` sets D2 = 0x0F, so requests round to **16 bytes**
(line 4390). `l03104` sets D2 = 0x07, so it rounds to **8 bytes** (line
4538). Growth through the transient gap rounds to **512 bytes** (`l0320c`).

Zeroing: `l02fae` clears from block base + 4 to base + size (lines 4393–4400).
The size longword at base + 0 is kept.

Caller evidence: the channel open at `l00b56` calls MM.ALCHP with D1 = 0x00E4
(line 1352) and tests `bne`. The open path at `l00dce` calls MM.ALCHP with
D1 = 0x0080 (line 1643) and releases with MM.RECHP on error (line 1670).

## 5. Three-column comparison

| Topic | Historical QDOS / JS ROM | R3 native | Deliberately deferred |
|---|---|---|---|
| **Allocation source** | Common heap, first fit; transient gap grown in 512-byte steps when full | Fixed arena from measured RAM map, first fit | Transient gap, 512-byte growth, SV.FREE/SV.BASIC movement |
| **Rounding** | 16 bytes (ALCHP); 8 bytes (ALLOC) | 16 bytes, always | 8-byte ALLOC variant |
| **Zero-fill** | ALCHP clears; ALLOC does not | Always clears | Non-zeroing allocation |
| **Size record** | Size longword at block base | Native header with size, owner, magic, state | Bit-exact header layout |
| **Release** | MT.RECHP links and coalesces; may return the top to the transient gap | `mem_free`: validate, coalesce both sides, no gap return | Gap return |
| **Ownership** | Channel- or system-owned; job blocks have a 16-byte prefix (`tr1d19`) | Owner = tagged job ID or `KERNEL` on every block; blocks never move | Channel-owned blocks |
| **Moving** | BASIC area and job headers relocated (`l031e0`, `l031c8`) | None | Relocation and pointer fix-up |
| **Largest-free query** | `l030e0` (trap 1, unresolved code) | Not provided as a trap | Entire query |
| **Trap interface** | 68000 D/A registers; trap-1/trap-2 dispatch tables | i386 `int 0x30`, EAX function, EBX/ECX/EDX arguments, EAX result | QDOS trap numbers and register ABI |
| **Errors** | -3 out of memory; -15 bad parameter (trap 2, `l032a2`) | 0 OK; native codes for zero size, overflow, bad pointer, double free, owner, corruption | Full JS ROM error set |

## 6. Classification summary

| Item | Status |
|---|---|
| MM.ALCHP 16-byte zeroed allocation | approximated (no transient fallback) |
| MM.RECHP release using size longword | approximated (no gap return) |
| MM.ALLOC 8-byte non-zeroing variant | deferred (not exposed in R3) |
| MM.LINKFR external linking | deferred (internal coalescing only) |
| SV.CHPFR head sentinel with relative links | implemented (own layout) |
| Job-owned 16-byte prefix (`tr1d19`) | deferred (R3 uses a native header) |
| Transient gap and 512-byte growth | deferred |
| BASIC relocation, `l031e0` fix-up, USP adjust | deferred; not applicable to non-moving stacks |
| `l030e0` largest-free query | unresolved (outside the R3 ABI) |
| `MT.FREE` comment at line 4471 | recorded as misplaced; the code belongs to `l0305e` |

All four audit items now have file and line evidence. The only unresolved
item is the trap code and caller of `l030e0`. It stays outside R3, so the
audit exit criterion is met.
