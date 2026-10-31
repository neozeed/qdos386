# R2 observed external boundary (incomplete shell)

No QDOS386 kernel, graphics, keyboard or audio implementation is included.
The following native cdecl calls replace actual external operations in the
translated modules. A nonzero service result is an error/unsupported result.

| Call | Source operation | Contract |
|---|---|---|
| `platform_sound_ipc(packet)` | sound.s MT.IPCOM, trap #1 function 17 | Original 24-byte BEEP or 8-byte stop packet; returns service result |
| `platform_file_open(filename, mode, channelOut)` | save.s IO.OPEN, trap #2 function 1 | Original counted filename bytes; mode 1 read / 3 replace; native channel output |
| `platform_file_transfer(channel, buffer, bytes, write, transferredOut)` | save.s trap #3 functions 3/7 | Blocking original transfer; write low word nonzero means write; native transferred count |
| `platform_file_close(channel)` | save.s IO.CLOSE, trap #2 function 2 | Return original close result |
| `platform_keyboard_queue_pop()` | keyboard.s current queue / IO.QOUT ROM vector | 0 consumed one character; nonzero empty, absent or error; caller makes at most 128 attempts |

`commander_io` preserves 144-byte legacy reads, conditional four-byte EQA2 tail,
148-byte writes, short-transfer error -10 and transfer-error precedence over
close errors. The sound caller preserves the source's behavior of ignoring IPC
errors; test adapters record attempts and return -1, never pretend output worked.
Keyboard drain does not synthesize held keys.

The untranslated shell additionally uses the fixed packed screen at 0x20000,
MT.ALCHP/RECHP allocation/free, MT.DMODE query/set, MT.LPOLL/RPOLL clock hook,
MT.FREE memory statistics, MT.SUSJB pacing, MT.FRJOB exit, and MT.IPCOM keyboard
row reads. These are observed dependencies, not implemented or invented APIs.
The original text renderer and menu logic are internal assembly work, not an OS
boundary. They must not be replaced by a new shell or omitted as “platform code.”

The test surface is exactly 32,768 bytes, 128 bytes/row. Flight clipping is
x=0..255, y=0..191. The shell requests MODE 8. Pixel words used in this source
are 0x80c0, 0x2030, 0x080c, 0x0203 in big-endian order. Asset bytes are retained.
No VGA palette, scaler or presentation conversion is introduced. The full
shell text/presentation audit remains incomplete.
