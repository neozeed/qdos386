# R1 assembly ABI and translation rules

Public callable symbols retain the RC6 leading underscore and take 32-bit
cdecl arguments on the stack, first argument at entry ESP+4. Caller removes
arguments. EAX carries scalar results; EBX/ESI/EDI/EBP are preserved; EAX/ECX/EDX
and flags are volatile. None of these routines changes DF. Persistent fitting
state remains `_ship_fit[8]`, initial bytes 22,15,0,0,0,0,0,0.

C callers should use the prototypes in `include/elite386.h`. As in the source,
indices/pointers must designate the documented valid game objects. This is not
an input-validation rewrite. No OS services are called by the translated code.

| Data | R1 representation |
|---|---|
| INWK/ship positions and orientation | Original bytewise sign-magnitude representation |
| Commander CASH | Big-endian 32-bit, explicit 386 byte exchanges/rotations |
| Commander TALLY | Original little-endian two-byte counter |
| Universe count and heapUsed | Original big-endian 16-bit fields at 14 and 16 |
| Galaxy seeds | Original three little-endian word pairs |
| RNG | Original four bytes; result includes A/carry/X packing |
| Matrix output | Nine native i386 signed shorts, row order unchanged |
| Stack arguments, internal pointers, local price table | Native little-endian |

`sign_add` is an internal assembly helper, not a C function: AX and DX inputs,
AX result, DX/flags volatile, other registers preserved. New flight/tidy code
must adapt its calls to this contract. Original m68k internal register contracts
are not binary interfaces on i386. Other local helper names are module-local.

Original intermediate byte/word widths are retained where they affect results,
including the cargo ADC carry chain, controller pitch's inherited carry,
signed zero and 16-bit rotation overflow. Multiplications/divisions use integer
80386 instructions. No BSWAP (486), CMOV, SSE or x87 instructions are required.

One shared equivalent helper is reused in travel_arrive: the Ghy nearest-system
search calls chart_nearest with (96,96), preserving seed order, initial best
127 and first-strict-minimum policy. All eight galaxy arrivals pass the original
fixtures. This does not add a new search rule.

Test boundary: the a.out adapter changes relocation addends to the ELF convention
and removes public-name underscores for host C drivers. It does not regenerate
instructions or provide game functions. The QDOS deliverables are the original
GAS a.out objects. No full-game linkage, scheduling, display or in-OS execution
has been claimed by this milestone.
