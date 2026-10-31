#!/bin/sh
# R14: print the QEMU options that export the guest's COM1 (and optionally COM2) to the host.
#   usage: sh scripts/r14-serial-args.sh MODE [DIR]
#   MODE  tcp:PORT          COM1 = TCP server on 127.0.0.1:PORT (raw bytes; any free port >= 1024)
#         tcp                as above on a free port chosen now (printed on stderr)
#         pty                COM1 = a host pseudo-terminal; QEMU prints its name on stderr:
#                            "char device redirected to /dev/pts/N (label r14com1)" (QEMU 8.2 ignores pty path=)
#         tcp2:P1:P2         COM1 raw TCP on P1, COM2 telnet-mode TCP on P2 (IAC BREAK = a UART break)
#         none               no serial ports at all
# No fixed host path, no privileged port.  The guest sees a 16550A at 0x3F8 IRQ4 (and 0x2F8 IRQ3).
set -eu
M=${1:?usage}; D=${2:-.}
freeport() { python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1]); s.close()'; }
case $M in
tcp) P=$(freeport); echo "COM1: tcp 127.0.0.1:$P" >&2; echo "-serial tcp:127.0.0.1:$P,server=on,wait=off" ;;
tcp:*) P=${M#tcp:}; [ "$P" -ge 1024 ] || { echo "port must be >= 1024" >&2; exit 2; }; echo "-serial tcp:127.0.0.1:$P,server=on,wait=off" ;;
tcp2:*) P1=$(echo "$M" | cut -d: -f2); P2=$(echo "$M" | cut -d: -f3)
        echo "-serial tcp:127.0.0.1:$P1,server=on,wait=off -serial tcp:127.0.0.1:$P2,server=on,wait=off,telnet=on" ;;
pty) echo "-chardev pty,id=r14com1 -serial chardev:r14com1" ;;
none) echo "-serial none" ;;
*) echo "unknown mode $M" >&2; exit 2 ;;
esac
