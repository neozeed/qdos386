# Validation of C port R1

Built from the supplied GCC 2.95.3 R3 source on Linux x86-64, using the supplied
JAS/LD/runtime, and executed as a native 68000 QDOS application in the supplied
QLBox R24 with 640 KB RAM. The production executable was compiled at `-O2`.
Tests use loopback peers only. No public BBS or external account is involved.

## Executed checks

| Area | Evidence |
| --- | --- |
| Release ZIP | Supplied QDOS UnZip restores byte-identical executable, type 1 and correct dataspace; extracted program boots |
| Native startup and display | Normal-speed and 8× QDOS screen boot; actual CON/SCR channels and child jobs; black/white paper checked in screen RAM |
| Screen parser | Native QDOS probe: CSI and VT52 positions, movement, save/restore, tabs, out-of-range fallback, bottom scrolling, colour mapping, inverse idempotence and ignored private CSI |
| Logging | Native raw/cooked byte comparisons; F3 Log open and close against a live peer |
| Telnet | Native file-driven byte checks plus live fragmented IAC, BINARY/SUPGA, ECHO/unsupported-option refusal, NAWS, ANSI terminal type, and cursor-position reply |
| Binary transparency | All 256 outgoing byte values; escaped incoming IAC; CRC known vector |
| Keys and queue | All ANSI/general key mappings, full queue rejection and wraparound |
| Command tail | Real Toolkit II invocation with SER2I device override |
| Inherited channels | 1-channel same-I/O, 2-channel separate I/O, 3-channel I/O plus cooked log, and rejection of 4 channels; clean exit |
| F3 interaction | Connect using edited hostname; logging; text transmission with CR/LF conversion and an unterminated last line |
| XMODEM receive | CRC and checksum fallback, corrupted-block retry, duplicate-block handling, >32 KB buffering and sequence rollover |
| XMODEM-1K receive | 1 KB data blocks and expected final padding |
| YMODEM receive | Exact length, executable type/dataspace/extra header word, retries, buffered and direct writes |
| YMODEM-G receive | Streaming reception without per-block ACKs |
| XMODEM send | CRC and checksum, retransmission and sequence rollover |
| XMODEM-1K send | 1 KB / short final blocks, retransmission |
| YMODEM send | Bare filename, descriptor wire length/date, executable header, payload, EOT and empty batch terminator |
| YMODEM-G send | Streaming upload and end-of-batch handshake |
| YMODEM batch | Multiple received files and `_dup1` duplicate naming |
| Embedded data | 28 MenuConfig entries in three blocks, all relative offsets and defaults validated; exact 1,318 font bytes from IBMGR.ASM and all 256 original CRC-table entries |

The native probe reports 90 assertions with no failures; 63 of these exercise
individual queue insertions. This count is not a claim of 90 independent features.
Transfer payloads are compared byte-for-byte by an independent Python peer.
The final test transcript is in `validation/final-tests.log`.

## Repeat the tests

Build the supplied QLBox headless frontend first:

    make -C /path/to/qlbox qlbox-headless

Then, on Linux with a host C compiler and Python 3:

    QLBOX_ROOT=/path/to/qlbox \
    QDOS_CC=/path/to/qdos-gcc \
    python3 tests/run.py

The runner builds a small host harness against the supplied QLBox core, and
native QDOS probe/launcher programs. It does not patch the production program
with test-only behaviour. Test copies of the executable have their existing
configuration fields set to SER2I and WIN1_ so they can use a loopback server and
host-mounted test files. The distributed `qlterm` retains the original defaults.
Generated data, dumps and temporary native probes go in `tests/work/`.
The tiny BASIC command strings in tests only launch executables/load Toolkit II;
none of QLTerm's BASIC utilities have been ported or used as implementations.

## Limits

- Physical QL hardware, external serial expansions, Hermes split baud, disk/SD
  timing and maximum sustainable throughput have not been tested. The C build
  has larger code and stack requirements than the assembly original.
- Native `TCP_` channel opening is present but not live-tested: QLBox R24 exposes
  its TCP connection as a SER2 modem. The application's Telnet parser is tested
  over the raw serial transport and native QDOS files.
- Receive update-date calls are implemented. QLBox R24's host filesystem lacks
  IOF.DATE, so an actual received-file timestamp cannot be verified there.
  Upload descriptor date conversion is byte-checked.
- MenuConfig's on-disk block is structurally checked, and settings are patched
  through those fields in the tests. The external MenuConfig application itself
  was not supplied and its interactive editing UI has not been exercised.
- Real network/subdirectory filename stripping, DATA_USE fallback, larger screen
  geometries and unexpanded 128 KB machines remain field-test items.
- The supplied assembly was inspected and retained as the porting reference;
  this is not a differential execution proof against a freshly assembled b2
  original. The nested old b1 executable was not treated as the b2 oracle.

See PORTING.md for explicit corrections rather than silent claims of identical
bug behaviour. There is no ZMODEM or SSH implementation hidden behind the tests.
