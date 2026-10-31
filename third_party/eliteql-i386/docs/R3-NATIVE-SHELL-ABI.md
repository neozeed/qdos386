# Native shell conventions

Public R2 core functions retain their frozen cdecl interface. Internal shell
control paths use EAX for D0, EDX for D1, EBX for D2 and short-lived menu D4/D6,
EBP for D7, ECX for A0, ESI for the current INWK/text pointer, and EDI for the
current renderer cache. Temporary A1/A2/D3/D5 allocations are source-region
specific; they do not constitute a persistent 68000 register bank. The cargo
page reloads its cursor from the authoritative item index. Original all-register
save regions use PUSHAL/POPAL. DBRA lowering preserves flags.

`asm_text` accepts ESI=text, EBX=y; `asm_text_at` additionally EBP=x.
`asm_decimal` accepts ECX=destination, EAX=value, EDX=width. These adapters invoke
the independently tested cdecl leaves. `asm_keyrow(row)` and
`asm_dust_run(operation)` use cdecl slots; the latter preserves all hardware
registers as the original wrapper did. Spawning accepts ESI=parent and
EDI=cache, returns ECX=child pointer or zero, and preserves EBX/ESI/EDI/EBP.
`uni_allocate` accepts EAX=original ship type and returns EAX=zero-based slot or
negative error, preserving other native registers.

Numeric words, longs and pointers are native little endian. Original serialized
commander CASH, save header/checksum, counted filenames, universe-list words,
encounter output counts/types, sound records and packed QL pixels retain original byte order. INWK records
remain raw sign/magnitude bytes. Renderer/cache numeric fields and model headers
are native, matching R2. Source byte accesses into numeric words are translated
explicitly, including `asm_old1/2/7+1` and `flow_random_carry`.

The platform's allocation boundary returns EAX=pointer and EDX=error to match
startup's two-result dependency. Other platform entries use cdecl. An optional
`_shell_platform_poll` pointer is null for deterministic headless tests and is
installed by the OS entry; it updates the original poll clock and presents the
private packed buffer at existing sound-poll/yield boundaries. It does not
execute gameplay. No kernel API or kernel implementation is added.
