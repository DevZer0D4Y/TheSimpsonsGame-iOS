#!/usr/bin/env python3
"""Copy the game files the phone is missing into the app's Documents/game.

Lists what is already on the device, compares sizes with the extracted game,
and copies only missing or incomplete files: a folder in one go when nothing of
it is on the phone yet, single files otherwise, smallest first. Language
folders other than the kept one (movies/<lang>, audiostreams/<lang>) and the
console's $systemupdate folder are skipped.

  sync-game-data.py <extracted game> [--device UDID] [--bundle ID] [--lang it]
                    [--dry-run]
"""
import argparse
import json
import os
import subprocess
import sys
import tempfile

LANG_DIRS = ('movies', 'audiostreams')
LANGS = {'en', 'it', 'es', 'fr', 'de', 'ja', 'ko', 'zh', 'pt', 'pl', 'ru', 'nl'}


def device_files(device, bundle):
    with tempfile.NamedTemporaryFile(suffix='.json', delete=False) as tmp:
        path = tmp.name
    subprocess.run(['xcrun', 'devicectl', 'device', 'info', 'files', '--device', device,
                    '--domain-type', 'appDataContainer', '--domain-identifier', bundle,
                    '--json-output', path], check=True, capture_output=True)
    files = json.load(open(path))['result']['files']
    os.remove(path)
    out = {}
    for f in files:
        p = f['relativePath']
        if p.startswith('Documents/game/') and not f['resources']['isDirectory']:
            out[p[len('Documents/game/'):]] = f['metadata'].get('size', 0)
    return out


def wanted_files(root, lang):
    for dirpath, dirnames, filenames in os.walk(root):
        rel = os.path.relpath(dirpath, root)
        parts = [] if rel == '.' else rel.split(os.sep)
        if parts and parts[0] == '$systemupdate':
            dirnames[:] = []
            continue
        if len(parts) >= 2 and parts[0] in LANG_DIRS and parts[1] in LANGS and parts[1] != lang:
            dirnames[:] = []
            continue
        for name in filenames:
            if name.startswith('._') or name == '.DS_Store':
                continue
            full = os.path.join(dirpath, name)
            yield '/'.join(parts + [name]), os.path.getsize(full)


def copy(device, bundle, source, dest):
    r = subprocess.run(['xcrun', 'devicectl', 'device', 'copy', 'to', '--device', device,
                        '--domain-type', 'appDataContainer', '--domain-identifier', bundle,
                        '--source', source, '--destination', 'Documents/game/' + dest],
                       capture_output=True, text=True)
    if r.returncode:
        tail = (r.stderr or r.stdout).strip().split('\n')[-2:]
        print(f'  FAILED {dest}: {" ".join(t.strip() for t in tail)}', flush=True)
        return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('root')
    ap.add_argument('--device', default=os.environ.get('DEVICE'),
                    help='paired device identifier, or set DEVICE')
    ap.add_argument('--bundle', default='com.devz.simpsons')
    ap.add_argument('--lang', default='it')
    ap.add_argument('--dry-run', action='store_true')
    a = ap.parse_args()
    if not a.device:
        ap.error("pass --device or set DEVICE to your paired iPhone identifier")

    have = device_files(a.device, a.bundle)
    want = dict(wanted_files(a.root, a.lang))
    missing = {p: s for p, s in want.items() if have.get(p) != s}
    print(f'{len(want)} files wanted, {len(have)} on the phone, {len(missing)} to copy '
          f'({sum(missing.values()) / 2**30:.2f} GB)', flush=True)

    # Folders with nothing on the phone go as one copy each.
    def folder_of(p):
        return p.rsplit('/', 1)[0] if '/' in p else ''
    units = {}
    for p, s in missing.items():
        unit, folder = p, folder_of(p)
        # Never above a language folder's parent: it also holds the skipped languages.
        while folder and folder not in LANG_DIRS:
            if not any(h == folder or h.startswith(folder + '/') for h in have):
                unit = folder
            folder = folder_of(folder)
        units.setdefault(unit, 0)
        units[unit] += s
    order = sorted(units.items(), key=lambda kv: (kv[0].startswith('movies'), kv[1]))
    for unit, size in order:
        print(f'{"(dry) " if a.dry_run else ""}copy {unit} ({size / 2**20:.1f} MB)', flush=True)
        if not a.dry_run and not copy(a.device, a.bundle, os.path.join(a.root, unit), unit):
            return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
