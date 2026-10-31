# Source correspondence and porting notes

The reference is the attached `QLTerm-working` source, whose MAIN.ASM and README
identify version 3.00b2. Older nested ZIP releases and BASIC utilities were not
used as replacement implementations.

| Assembly source / area | C implementation |
| --- | --- |
| MAIN.ASM START, windows, child jobs, main loop | src/main.c |
| MAIN.ASM K_GEN, K_ASC, K_VT52, K_ANSI | src/serio.c key_send |
| MAIN.ASM STATUS, GETMENU, GETREPLY, F3 commands | src/main.c |
| MAIN.ASM SENDCMD, CALL, OPEN_SER, SETBAUD | src/main.c |
| MAIN.ASM MultiConfig sections | tools/generate_data.py → src/config.c/config.h |
| SCREEN.ASM controls, ESCTBL, A_CMDTBL, SGR | src/screen.c |
| SCREEN.ASM log filtering and screen queue consumer | src/screen.c |
| serio.asm RXCHAR / Telnet state | src/serio.c rxchar |
| serio.asm TXSTR, TXCHR, PUT_RXQ, SERJOB | src/serio.c |
| FILEXFER.ASM download/upload state and framing | src/filexfer.c |
| FILEXFER.ASM QDOS header and YMODEM descriptor | src/filexfer.c |
| FILEXFER.ASM file open/name handling; utils.asm | src/platform.c |
| CRCCITT.ASM | src/filexfer.c crc16 |
| IBMGR.ASM | mechanically generated src/font.c |

The architecture still uses three QDOS jobs. The screen and receive workers
execute C on private QDOS stacks and share the relocated application state.
The C state machines replace register-held state and assembler macros. The QDOS
screen driver still owns cursor geometry and bitmap rendering. No alternative
ANSI library or filesystem emulation has been substituted.

## Implementation differences and corrections

This is source-derived functionality, not instruction-by-instruction bug
emulation. The following differences are explicit:

- `ut_scr()` in the supplied libc source calls address $C8 rather than the vector
  stored there. It crashed the first guest build. The port uses `IO.OPEN` plus
  `SD.WDEF` and the colour/clear calls for both window types. The bundled compiler
  and emulator remain unchanged.
- The supplied libc declares screen colour arguments as bytes although its
  trap stubs fetch words. The port calls the word ABI explicitly so undefined
  argument padding cannot become a stipple/colour value. Screen bitmap checks
  cover black and white paper in addition to logical parser state.
- The C runtime converts the command tail in place before `main`. Reading it as
  a still-counted QDOS string loses the first two letters. The port reads the
  converted string while preserving the original inherited-channel array.
- Buffer indices use full-width values. Byte count limits on string traps remain
  32767. TAS locks protect the receive producers and serial writers; workers are
  stopped outside locked regions. Full queues do not overwrite unread text.
- File transfer traffic bypasses local echo. Protocol framing remains binary
  regardless of the terminal echo setting. Telnet data still doubles IAC.
- Received block-number complements are validated. Retransmitted previous blocks
  are ACKed without writing them twice, including after 255→0 rollover. Corrupt
  checksums/CRCs follow the existing retry/cancel path.
- An executable's 30-byte QDOS prefix counts toward wire block length, including
  short last blocks. File payload, type, dataspace and additional header word are
  preserved. Malformed prefix lengths and overlong paths are bounded.
- Text transmission preserves an unterminated final line. The assembly's
  IO.FLINE path discarded bytes returned together with EOF.
- Configured download buffering is honoured on non-RAM devices. The source's
  startup branch contradicts its RAM-disk comment and clears it for non-RAM
  devices. No equivalent accidental clearing is retained.
- Update dates are applied after received data is written, then the file closes.
  The original applied the date at descriptor receipt. The QDOS 1961 / Unix 1970
  offset is unchanged (283996800 seconds).
- Download names use the existing `_dup1`, `_dup2`, etc. mechanism, with bounds
  checking. File operations and inherited channels still use QDOS IDs directly.
- MenuConfig port descriptions match the port constants, as described in README.

The source has intentional historical behaviour retained here: VT52 and CSI
input are accepted in all terminal modes; mode controls outgoing keys and the
Telnet terminal type. No general VT100 emulator, DEC scrolling regions, bold,
SSH or ZMODEM has been introduced. Upload remains a single selected file per
command, followed by the YMODEM batch terminator.

## Unverified platform paths

Native `TCP_` opening, hardware serial timing, Hermes commands, MenuConfig's
interactive UI, expanded screen sizes, Toolkit II DATA_USE directory fallback,
network/subdirectory filename stripping, and real-media write delays need field
testing on the relevant QL systems. Their code paths are present; emulated
SER2 and the supplied host filesystem cannot establish every hardware claim.
