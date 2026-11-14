#!/usr/bin/env python3
"""Populate a deterministic mkfs.fat FAT16 image from an explicit payload list.

Uses the repository's make-fat16.py entry encoder; independent validation must
use fsck.fat and r11m-fatwalk.py, which share no allocation code with this writer.
"""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path, PurePosixPath
import re
import struct
import subprocess


def load_entries():
    spec = importlib.util.spec_from_file_location('fat_records', Path(__file__).with_name('make-fat16.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.entry


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('stage', type=Path)
    ap.add_argument('file_list', type=Path)
    ap.add_argument('output', type=Path)
    args = ap.parse_args()
    rows = json.loads(args.file_list.read_text())
    tree = {'children': {}, 'cluster': 0, 'name': ''}
    seen = set()
    manifest = []
    for row in rows:
        path = PurePosixPath(row['destination'])
        if not path.is_absolute() or '..' in path.parts or str(path).upper() != str(path):
            ap.error('destination must be an absolute uppercase path: ' + str(path))
        if str(path).casefold() in seen:
            ap.error('duplicate destination: ' + str(path))
        seen.add(str(path).casefold())
        data = (args.stage / row['source']).read_bytes() if row.get('source') else b''
        node = tree
        for part in path.parts[1:-1]:
            node = node['children'].setdefault(part, {'name': part, 'children': {}})
            if 'children' not in node:
                ap.error('file/directory collision: ' + str(path))
        if path.name in node['children']:
            ap.error('file/directory collision: ' + str(path))
        node['children'][path.name] = {'name': path.name, 'data': data}
        manifest.append(dict(row, size=len(data), sha256=hashlib.sha256(data).hexdigest()))
    for directory in ['/TMP', '/HOME', '/GAMES/HACK/SAVE']:
        node = tree
        for part in PurePosixPath(directory).parts[1:]:
            node = node['children'].setdefault(part, {'name': part, 'children': {}})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # Never overwrite an existing canonical/reference volume.
    with args.output.open('xb') as f:
        f.truncate(32 * 1024 * 1024)
    subprocess.run(['mkfs.fat', '--invariant', '-F', '16', '-s', '2', '-r', '512',
                    '-f', '2', '-S', '512', '-R', '1', '-n', 'WIN1', '-i', '52324144',
                    str(args.output)], check=True)
    image = bytearray(args.output.read_bytes())
    bps, spc, reserved, nfats, rootent, total16, media, spf = struct.unpack_from('<HBHBHHBH', image, 11)
    total = total16 or struct.unpack_from('<I', image, 32)[0]
    root_sectors = (rootent * 32 + bps - 1) // bps
    data_start = reserved + nfats * spf + root_sectors
    clusters = (total - data_start) // spc
    assert bps == 512 and nfats == 2 and 4085 <= clusters <= 65524
    cbytes = bps * spc
    fat = bytearray(image[reserved*bps:(reserved+spf)*bps])
    entry = load_entries()
    cursor = 2
    date = ((2026 - 1980) << 9) | (11 << 5) | 12

    def record(alias, attr, cluster, size):
        r = entry(alias, attr, cluster, size)
        struct.pack_into('<H', r, 16, date)  # creation date
        struct.pack_into('<H', r, 18, date)  # last access
        struct.pack_into('<H', r, 24, date)  # modification date; all times 00:00
        return r

    def allocate(size):
        nonlocal cursor
        count = math.ceil(size / cbytes)
        if not count:
            return 0
        first = cursor
        if cursor + count > clusters + 2:
            raise ValueError('32 MiB volume capacity exceeded; no payload was silently dropped')
        for c in range(first, first + count):
            struct.pack_into('<H', fat, c*2, c+1 if c < first+count-1 else 0xffff)
        cursor += count
        return first

    def short_names(node):
        used = set()
        children = sorted(node['children'].values(), key=lambda n: n['name'])
        # Reserve literal 8.3 names first to keep aliases collision-free.
        for n in children:
            name = n['name']
            if re.fullmatch(r'[A-Z0-9_-]{1,8}(\.[A-Z0-9_-]{1,3})?', name):
                base, _, ext = name.partition('.')
                n['alias'] = (base.ljust(8)+ext.ljust(3)).encode('ascii')
                used.add(n['alias'])
        for n in children:
            if 'alias' not in n:
                base, _, ext = n['name'].rpartition('.')
                if not base:
                    base, ext = ext, ''
                base = re.sub('[^A-Z0-9_]', '_', base)
                for i in range(1, 1000000):
                    suffix = '~'+str(i)
                    alias = (base[:8-len(suffix)]+suffix).ljust(8)+ext[:3].ljust(3)
                    if alias.encode('ascii') not in used:
                        n['alias'] = alias.encode('ascii'); used.add(n['alias']); break
                else:
                    raise ValueError('no unique short alias')
                n['long'] = True
        return children

    def lfns(n):
        if not n.get('long'):
            return []
        name = list(struct.unpack('<'+'H'*len(n['name']), n['name'].encode('utf-16le'))) + [0]
        name += [0xffff] * (-len(name) % 13)
        checksum = 0
        for b in n['alias']:
            checksum = (((checksum & 1) << 7) + (checksum >> 1) + b) & 255
        records = []
        for i in reversed(range(len(name)//13)):
            r = bytearray(32); r[0] = i+1 | (0x40 if i == len(name)//13-1 else 0)
            r[11] = 15; r[13] = checksum
            packed = struct.pack('<13H', *name[i*13:(i+1)*13])
            r[1:11], r[14:26], r[28:32] = packed[:10], packed[10:22], packed[22:]
            records.append(r)
        return records

    def plan(node, root=False):
        children = short_names(node)
        nrecords = 1 if root else 2  # root volume label or . / ..
        nrecords += sum(1 + len(lfns(n)) for n in children) + 1
        if root:
            if nrecords > rootent:
                raise ValueError('root directory full')
        else:
            node['cluster'] = allocate(nrecords * 32)
        for child in children:
            if 'children' in child:
                plan(child)
            else:
                child['cluster'] = allocate(len(child['data']))

    def put(cluster, data):
        if cluster:
            off = (data_start + (cluster-2)*spc)*bps
            image[off:off+len(data)] = data

    def emit(node, parent=0, root=False):
        if root:
            records = [record(b'WIN1       ', 8, 0, 0)]
        else:
            records = [record(b'.          ', 16, node['cluster'], 0), record(b'..         ', 16, parent, 0)]
        for n in sorted(node['children'].values(), key=lambda n: n['name']):
            isdir = 'children' in n
            records += lfns(n)
            records.append(record(n['alias'], 16 if isdir else 32, n['cluster'], 0 if isdir else len(n['data'])))
            if isdir:
                emit(n, node['cluster'])
            else:
                put(n['cluster'], n['data'])
        data = b''.join(records) + bytes(32)
        if root:
            off = (reserved + nfats*spf)*bps
            image[off:off+len(data)] = data
        else:
            put(node['cluster'], data)

    plan(tree, True)
    emit(tree, root=True)
    for i in range(nfats):
        off = (reserved+i*spf)*bps
        image[off:off+len(fat)] = fat
    args.output.write_bytes(image)
    result = {'image_sha256': hashlib.sha256(image).hexdigest(), 'bytes': len(image),
              'bps': bps, 'spc': spc, 'reserved': reserved, 'fats': nfats,
              'root_entries': rootent, 'sectors_per_fat': spf, 'data_start': data_start,
              'clusters': clusters, 'allocated_clusters': cursor-2, 'files': sorted(manifest, key=lambda r:r['destination'])}
    args.output.with_suffix('.manifest.json').write_text(json.dumps(result, indent=2)+'\n')
    print(f"{args.output}: {len(manifest)} files, sha256={result['image_sha256']}")


if __name__ == '__main__':
    main()
