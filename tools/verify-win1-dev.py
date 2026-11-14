#!/usr/bin/env python3
"""Read-only independent FAT walker + exact extracted-file manifest verification."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('image',type=Path)
    ap.add_argument('manifest',type=Path)
    args=ap.parse_args()
    spec=importlib.util.spec_from_file_location('fatwalker',Path(__file__).with_name('r11m-fatwalk.py'))
    walker=importlib.util.module_from_spec(spec); spec.loader.exec_module(walker)
    manifest=json.loads(args.manifest.read_text())
    volume=walker.Vol(str(args.image))
    errors,warnings,nodes=walker.walk(volume,True)
    actual={}
    for node in nodes:
        if node['kind']!='F': continue
        path=str(Path(node['path']).with_name(node['lfn'] or Path(node['path']).name))
        payload=walker.read_file(volume,node)
        actual[path]=(len(payload),hashlib.sha256(payload).hexdigest())
    expected={row['destination']:(row['size'],row['sha256']) for row in manifest['files']}
    if len(expected)!=len(manifest['files']): errors.append('duplicate manifest destination')
    for path in sorted(set(actual)|set(expected)):
        if actual.get(path)!=expected.get(path): errors.append('payload mismatch: '+path)
    digest=hashlib.sha256(args.image.read_bytes()).hexdigest()
    if digest!=manifest['image_sha256']: errors.append('whole-image SHA-256 mismatch')
    for message in errors+warnings: print(message)
    print(f'{len(actual)} files independently extracted; {len(errors)} errors, {len(warnings)} warnings; SHA-256 {digest}')
    return int(bool(errors or warnings))


if __name__=='__main__':
    raise SystemExit(main())
