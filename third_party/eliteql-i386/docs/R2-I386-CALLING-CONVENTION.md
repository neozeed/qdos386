# R2 native i386 ABI (work in progress)

Public symbols use the historical leading underscore and i386 cdecl: arguments
are 32-bit stack slots, first at ESP+4 on entry; caller removes arguments;
EAX holds an integer return; EBX/ESI/EDI/EBP and ESP are preserved. EAX/ECX/EDX
and arithmetic flags are volatile. No floating point, SSE, CMOV, BSWAP or
post-80386 instructions are required. Direction flag is clear at public exits.
Routines with original void results do not promise an EAX value.

Signed byte/word operations retain truncation before the source branch. Native
word multiply/divide explicitly reproduces source rounding and quotient overflow
where used. Sign-magnitude negative zero is preserved by movement helpers.

## Storage

| Data | Representation |
|---|---|
| INWK, controls, cargo, laser, travel, planet, RNG, dust and cloud records | Original bytes; source offsets unchanged |
| Commander CASH | Original big-endian 32-bit bytes |
| Commander tally | Original separate low/high bytes |
| Universe-list counters | Original big-endian words inside byte state |
| Sound duration/tick/last-pitch/hardware fields | Original big-endian bytes |
| Screen pack run count/value | Original big-endian words |
| Packed MODE 8 bitmap, dashboard and bulb assets | Original byte order, never native scalar words |
| Model words, face products and blueprint pointers | Native little-endian words/longs/pointers |
| Matrix/position/projection short arrays | Native little-endian shorts |
| Renderer cache header, cloud cached count at offset 56 | Native words; endpoint/point records remain bytes |
| Cloud projected vertex records starting at byte 7 | Original little-endian coordinate bytes |
| Commander file contents / filename counted string | Original serialized bytes |

The future shell must obey these distinctions. A blanket byte-swap or blanket
little-endian conversion is incorrect. There is no QDOS386 call ABI adapter yet.

## Internal entry points

- `sign_add`: AX/DX sign-magnitude words -> AX. DX/flags volatile, other registers preserved.
- `tidy_mult`: low EAX/EDX bytes -> EAX encoded word. EBX/EDX scratch.
- `normalize_vector`: ESI points to three high bytes at stride 2; advances ESI by 6; EAX/EDX/ECX/EBX/EDI scratch.
- `clip_segment`: cdecl pointer to four native signed shorts; returns accept in EAX, updates only on acceptance; normal callee saves.
- `xor_segment`: EAX=x0, EDX=y0, EBX=x1, ECX=y1, ESI=packed framebuffer. All general registers preserved; flags volatile.
- Dust generated body: ESI=state, EDI=frame; AL/BL/CL are its source byte arithmetic temporaries, DL carry, DH saved N/Z value, AH scratch; EBP is address-index scratch. Native calls and branches, no runtime instruction decoder. `_dust_run` preserves the public ABI.

Other local helpers are private to their modules. Source comments describe their
register allocation. Global scratch in the original renderer and scanner means
those modules remain non-reentrant; this checkpoint does not add concurrency.
