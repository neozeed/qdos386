#!/usr/bin/env python3
"""Build a private disposable FAT16 recovery + FAT32 system disk under /tmp.

Run from an isolated source extraction. The accepted FAT16 fallback is read only;
all populated files use the qualified writer and are independently checked.
Historical applications are copied byte-for-byte, with their long names retained.
"""
import argparse
import hashlib
import importlib.util
import json
import struct
import subprocess
import sys
from pathlib import Path
sys.dont_write_bytecode = True


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ('output', 'fallback', 'package', 'apps', 'channels', 'live', 'populator'):
        ap.add_argument('--' + name, type=Path, required=True)
    ap.add_argument('--sdk-libs', type=Path)
    a = ap.parse_args()
    root = Path(__file__).resolve().parents[2]
    out = a.output.resolve()
    assert not a.output.exists() and not a.output.is_symlink()
    out.mkdir()

    def run(cmd, log):
        p = subprocess.run(list(map(str, cmd)), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (out / log).write_bytes(p.stdout)
        if p.returncode:
            raise RuntimeError('%s exited %d; see %s' % (cmd, p.returncode, out / log))

    boot = out / 'recovery.disposable.img'
    run([sys.executable, root / 'validation/r7/install-image.py',
         '--input', a.fallback, '--output', boot, '--package', a.package,
         '--apps', a.apps, '--probe', a.channels, '--probe-name', 'F32IO.AOU'], 'install.json')
    fat16 = out / 'recovery-partition.disposable.img'
    fat16.write_bytes(boot.read_bytes()[2048 * 512:])
    walker = module('walker', root / 'tools/r11m-fatwalk.py')
    volume = walker.Vol(fat16)
    errors, lost, nodes = walker.walk(volume, True)
    assert not errors and not lost, (errors, lost)
    payload = out / 'payload'
    payload.mkdir()
    manifest, hashes, names = [], {}, {'/': '/'}
    for node in nodes:
        path = node['path']
        target = str(Path(names[str(Path(path).parent)]) / (node['lfn'] or Path(path).name))
        names[path] = target
        file = payload / path.lstrip('/')
        if node['kind'] == 'D':
            file.mkdir(parents=True, exist_ok=True)
            manifest.append('D\t' + target)
        else:
            file.parent.mkdir(parents=True, exist_ok=True)
            body = walker.read_file(volume, node)
            if a.sdk_libs and path.startswith('/SDK/LIB/'):
                library = {'LIBGCC.A':'libgcc-qd.a','LIBGCC~1.A':'libgcc-qd.a'}.get(Path(path).name, Path(path).name.lower())
                fresh = a.sdk_libs / library
                if fresh.exists():
                    assert fresh.read_bytes() == body, 'SDK payload differs from fresh source build: ' + path
                    body = fresh.read_bytes()
            file.write_bytes(body)
            hashes[path] = sha(body)
            manifest.append('F\t' + target + '\t' + str(file))
    manifest.append('F\t/F32LIVE.AOU\t' + str(a.live.resolve()))
    hashes['/F32LIVE.AOU'] = sha(a.live.read_bytes())
    (out / 'payload.tsv').write_text('\n'.join(manifest) + '\n')
    (out / 'payload-hashes.json').write_text(json.dumps(hashes, indent=2) + '\n')
    (out / 'longnames.json').write_text(json.dumps(names, indent=2) + '\n')
    fat32 = out / 'system-partition.disposable.img'
    with fat32.open('xb') as f:
        f.truncate(196608 * 512)
    run(['mkfs.fat', '--invariant', '-F', '32', '-s', '1', '-n', 'QDOS32', fat32], 'mkfs.txt')
    with fat32.open('r+b') as f:
        for sector in (0, 6):
            f.seek(sector * 512 + 28)
            f.write(struct.pack('<I', 67584))
    run([a.populator, '--disposable', fat32, '--manifest', out / 'payload.tsv'], 'populate.txt')
    run(['fsck.fat', '-n', fat16], 'fsck-recovery.txt')
    run(['fsck.fat', '-n', fat32], 'fsck-system.txt')
    decoded = module('decoder', root / 'validation/r7/fat32-decode.py').inspect(fat32)
    assert not decoded['errors'], decoded['errors']
    assert set(decoded['files']) == set(hashes)
    assert all(decoded['files'][p]['sha256'] == h for p, h in hashes.items())
    (out / 'decoder.json').write_text(json.dumps(decoded, indent=2) + '\n')
    disk = out / 'qdos386-r7b2-fat32.disposable.img'
    with disk.open('xb') as f:
        f.write(boot.read_bytes())
        f.truncate(264192 * 512)
        f.seek(462)
        f.write(struct.pack('<B3sB3sII', 0, b'\xfe\xff\xff', 12, b'\xfe\xff\xff', 67584, 196608))
        f.seek(67584 * 512)
        with fat32.open('rb') as src:
            while True:
                data = src.read(1048576)
                if not data:
                    break
                if any(data):
                    f.write(data)
                else:
                    f.seek(len(data), 1)
    receipt = dict(disk=str(disk), sha256=sha(disk.read_bytes()), files=len(hashes),
                   fallback_sha256=sha(a.fallback.read_bytes()), package_sha256=sha(a.package.read_bytes()),
                   fat16_start=2048, fat16_sectors=65536, fat32_start=67584, fat32_sectors=196608)
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))


if __name__ == '__main__':
    main()
