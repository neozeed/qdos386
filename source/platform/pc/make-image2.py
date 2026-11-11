#!/usr/bin/env python3
"""R11K: build a Tiny386/QEMU raw disk image for the multi-transfer boot sector (boot2.s).

Usage: make-image2.py boot2.bin kernel.raw xfer.bin out.img [--max-sectors N]

Layout (unchanged from R11J): sector 0 boot, sectors 1..K kernel, K+1..K+X transfer area.
Patched into the boot sector: K at 0x1C2, X at 0x1C4, and at 0x1C6 a 32-bit checksum of all K+X
sectors (rotate-left-1-and-add over little-endian dwords), which the loader verifies after reading.

Limits (they mirror the loader's own run-time checks, so a bad image is refused here first):
  K >= 1, 1 <= X <= 512, K + X <= 896  (low-memory buffer 0x10000..0x7FFFF)
The legacy make-image.py keeps its 120-sector limit and is not used for this loader.
"""
import pathlib, struct, sys

MAXSEC = 896      # boot2.s: 448 KiB buffer
MAXXFER = 512     # the transfer area copy target has 256 KiB

def checksum(data):
    assert len(data) % 4 == 0
    s = 0
    for (w,) in struct.iter_unpack('<I', data):
        s = (s + w) & 0xFFFFFFFF
        s = ((s << 1) | (s >> 31)) & 0xFFFFFFFF
    return s

def build(boot, kern, xfer, max_sectors=MAXSEC):
    boot = bytearray(boot)
    if len(boot) != 512 or boot[0x1FE:0x200] != b'\x55\xaa':
        raise SystemExit('bad boot sector')
    if boot[0x1CA:0x1D0] != b'QBOOT2':
        raise SystemExit('not the R11K boot sector (tag QBOOT2 missing at 0x1CA): pair boot2.bin with make-image2.py only')
    if any(boot[0x1C2:0x1CA]):
        raise SystemExit('boot sector count/checksum fields are not zero (wrong boot sector?)')
    ksec = (len(kern) + 511) // 512
    xsec = (len(xfer) + 511) // 512
    if ksec == 0:
        raise SystemExit('empty kernel')
    if xsec == 0 or xsec > MAXXFER:
        raise SystemExit('transfer area must be 1..%d sectors (got %d)' % (MAXXFER, xsec))
    if ksec + xsec > max_sectors:
        raise SystemExit('kernel + transfer = %d sectors exceed the loader buffer (%d)' % (ksec + xsec, max_sectors))
    payload = kern.ljust(ksec * 512, b'\0') + xfer.ljust(xsec * 512, b'\0')
    csum = checksum(payload)
    boot[0x1C2:0x1C4] = ksec.to_bytes(2, 'little')
    boot[0x1C4:0x1C6] = xsec.to_bytes(2, 'little')
    boot[0x1C6:0x1CA] = csum.to_bytes(4, 'little')
    return bytes(boot) + payload, ksec, xsec, csum

if __name__ == '__main__':
    args = sys.argv[1:]
    maxs = MAXSEC
    if '--max-sectors' in args:
        i = args.index('--max-sectors'); maxs = int(args[i + 1]); del args[i:i + 2]
    boot_p, kern_p, xfer_p, out_p = map(pathlib.Path, args[:4])
    img, ksec, xsec, csum = build(boot_p.read_bytes(), kern_p.read_bytes(), xfer_p.read_bytes(), maxs)
    out_p.write_bytes(img)
    print('kernel bytes=%d sectors=%d transfer bytes=%d sectors=%d total=%d checksum=%08x' % (
        len(kern_p.read_bytes()), ksec, len(xfer_p.read_bytes()), xsec, ksec + xsec, csum))
