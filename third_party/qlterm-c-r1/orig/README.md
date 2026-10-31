# QLTerm C — R1

A native Sinclair QL / QDOS C port of **Jan Bredenbeek's QLTerm 3.00b2**,
using the supplied QDOS GCC 2.95.3 R3 compiler. This is a port of the supplied
assembly application, not a desktop terminal or a BASIC rewrite. The BASIC
programs are excluded. The original non-BASIC source is retained in `reference/`.

The application logic is C89-compatible C: menus, display parser, keyboard
translation, serial/Telnet processing, file operations, and X/YMODEM protocols.
The only inline assembly is the 68000 TAS operation used for job-safe locks.
The existing compiler startup and QDOS runtime provide the usual trap wrappers.
The child jobs have a six-byte QDOS jump entry written by their C creator.

**R1 is ready for field testing, not a claim of verified parity on every QL
expansion, serial interface, filesystem, or SMSQ/E emulator.** See
[VALIDATION.md](VALIDATION.md) for the executed tests and remaining limits.

## Build

The patched cross compiler must already have been built or installed. With an
installed compiler:

    make

Or with the supplied R3 compiler package, after running its `build.py`:

    make QDOS_CC=/path/to/QDOS-GCC-2.95.3-R3-HOST-CALL-FIX/bin/qdos-gcc

The result is `qlterm`, a native 68000 QDOS executable with an XTcc dataspace
trailer. The normal build uses the checked-in generated C data and needs no
Python. It uses `-O2`, the compiler wrapper's `-m68000 -msoft-float`, JAS, and
its normal QDOS linker. It does not require ELF, modern GAS, or a new runtime.

    make clean
    make QDOS_CC=/path/to/qdos-gcc

`make data` regenerates the font, CRC table and MenuConfig block from the
included recipe/reference sources; that optional step requires Python 3.

## Run on the QL

The release ZIP contains the `qltc/` directory and attaches a QDOS executable
header to `qlterm`. Extract it with
a QDOS-aware UnZip to preserve file type and dataspace on native media.
For an executable-only extraction, select `qltc/qlterm` with UnZip's `-j` option.
QLBox also recognises the XTcc trailer when the binary is copied to a host mount.
A generic host unzip followed by a byte-only copy to real QDOS media may lose
its executable header; use a QDOS-aware transfer/unzip method.

    EXEC WIN1_qlterm

Use a MODE 4 screen. As in the supplied source, MODE 8 is rejected. The default
terminal is 80 columns by 24 rows plus its status/command line. This build was
validated with **640 KB RAM** in QLBox. It uses more RAM than the assembly
application: a C runtime/stack and separate C stacks for the receive and screen
jobs, in addition to the configured buffers. A bare 128 KB QL is not claimed.

The program uses QDOS CON/SCR, serial/device channels and filesystem calls;
there are no QLBox diagnostic ports, host sockets, or emulator-specific memory
addresses in the application.

With Toolkit II, the raw command tail specifies an existing QDOS device:

    EX WIN1_qlterm;"SER2I"

The C runtime converts this tail to a C string before `main`; the port accounts
for that. The EX/EW inherited-channel conventions are retained:

| Channels | Meaning |
| --- | --- |
| 1 | Same channel for receive and transmit |
| 2 | Receive, transmit |
| 3 | Receive, log, transmit |
| 4 or more | Rejected |

## Run with the supplied QLBox R24

Mount the folder containing `qlterm` and start the normal screen frontend:

    qlbox --ram 640 --mount /path/to/qlterm-c --modem-mode raw

Then execute `WIN1_qlterm` from QDOS. **Do not use `qlrun`/`--run` for this
application:** that frontend intentionally lacks the QDOS screen operations
QLTerm uses.

The unchanged default interface is Telnet, which expects a QDOS `TCP_` driver.
The supplied QLBox R24 provides its TCP modem through **SER2**, not a native
`TCP_` device. Use F3, Interface, retain Echo Off, select SER2I, and confirm the
baud selection. Then F3, Connect and enter `hostname:port`; the existing modem
prefix/postfix produce the dial command. Alternatively type `ATDT host:port`
in the terminal. For ordinary Telnet servers, QLBox's default modem mode can
perform Telnet processing below SER2. Use `--modem-mode raw` for binary/raw peers.
Do not enable two layers of Telnet negotiation on the same connection.

The default download directory is `RAM2_`. If no RAM disk exists, use MenuConfig
to change it to an available directory, such as `WIN1_`. Nothing silently maps
that directory to a host path.

## Existing user interface

F3 opens the existing commands: **Connect, Download, Hangup, Interface, Log,
Options, Quit, Transmit, Upload**. The original unlisted **B** command changes
the download buffer size. Menus use left/right, Enter to accept, Escape to cancel.
Connect retains the previous host/phone entry and uses native QDOS line editing.
CTRL-X cancels transfers; Escape cancels text transmission.

Preserved features include:

- ASCII, VT52 and ANSI keyboard modes; the source's shared VT52/ANSI display
  parser; cursor save/restore and position reports; colour, underline, inverse
  video, scrolling/panning, and the exact IBMGR font data.
- SER1I/SER1H/SER2I/SER2H, a configured custom QDOS device, raw TCP and Telnet.
  Local echo/off/local-only, baud selections including NONE and Hermes 12H75,
  configurable modem strings, receive buffering and XON/XOFF backpressure.
- Telnet BINARY, ECHO, SUPGA, terminal-type and window-size handling, including
  escaped 0xFF in data. Unsupported options receive the original refusal policy.
- Raw/cooked logging, configurable CR/LF conversion, text-file transmission,
  configurable Enter and Backspace keys, and the existing QL key tables.
- XMODEM checksum/CRC, XMODEM-1K, YMODEM batch reception and YMODEM-G; padded
  XMODEM lengths, exact YMODEM lengths, QDOS executable headers, date-setting
  calls, duplicate download names, receive buffering and progress/throughput.
  Upload sends one selected file followed by the YMODEM end-of-batch block,
  matching the supplied implementation; there is no new batch-selection UI.
- Separate receive and screen jobs. Incoming text continues to display while
  the command menu uses the status line. These jobs stop during file transfers.

No ZMODEM, SSH, scrollback, GUI host application, or new terminal commands were
added. These were not implemented features of the supplied assembly.

## Configuration

The executable contains a real `<<QCFX>>02` MenuConfig block with the same
window, modem and other settings. Configure the **executable**, not the C text.
The old BASIC configurator is not included or required.

Defaults: Telnet; ANSI; Echo Off; baud 2400; receive buffer 64 KB; download buffer
32 KB; download directory RAM2_; Enter sends CR; Backspace sends DEL; cooked log;
CR/LF conversion and BEL enabled; default X/YMODEM choice YMODEM.

The port menu's values follow `QLTERM.IN` and the runtime interface table:
4=Custom, 5=TCP, 6=Telnet. The original MenuConfig labels for these three values
were inconsistent with that table; the C configuration labels match the actual
values. `PORT=6` remains the supplied default. The historical duplicate `QLTR`
configuration identifier is retained for compatibility.

See [PORTING.md](PORTING.md) for the source mapping and deliberate implementation
corrections. See [VALIDATION.md](VALIDATION.md) for testing instructions.

## Licence

GPL version 3 or later, derived from Jan Bredenbeek's QLTerm (1987–2026).
Original notices and GPL text are included. The C translation and test scripts
in this package are distributed under the same licence.
