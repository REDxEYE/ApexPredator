#!/usr/bin/env python3
"""Exercise named RTPC JSON export and output write errors through the CLI."""
import json
from pathlib import Path
import sqlite3
import struct
import subprocess
import sys
import tempfile


def main():
    binary = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix='apex-rtpc-json-') as temp:
        root = Path(temp)
        (root / 'steam_appid.txt').write_text('704270\n')
        initial = root / 'initial'
        initial.mkdir()
        asset_hash = 0x12345678
        prop_hash = 0x2468ACE0
        payload = (struct.pack('<4sIIIHH', b'RTPC', 1, 0xABCDEF01, 20, 1, 0)
                   + struct.pack('<IIB3x', prop_hash, 42, 1))
        (initial / 'test.tab').write_bytes(struct.pack('<4shhiIII', b'TAB\0', 2, 1, 2048,
                                                       asset_hash, 0, len(payload)))
        (initial / 'test.arc').write_bytes(payload)
        database = root / 'hashes.db'
        with sqlite3.connect(database) as db:
            db.executescript('CREATE TABLE kv(k INTEGER PRIMARY KEY,v TEXT NOT NULL) WITHOUT ROWID;'
                             'CREATE TABLE files(hash INTEGER PRIMARY KEY,name TEXT,size INTEGER NOT NULL,'
                             'parent INTEGER NOT NULL DEFAULT 0) WITHOUT ROWID;')
            db.execute('INSERT INTO kv VALUES (?,?)', (asset_hash, 'maps/test.epe'))

        def extract(output):
            return subprocess.run([str(binary), 'extract', str(root), f'0x{asset_hash:08X}',
                                   '--module', 'generation-zero', '-d', str(database),
                                   '-o', str(output)], capture_output=True, text=True)

        output = root / 'output'
        result = extract(output)
        assert result.returncode == 0, (result.stdout, result.stderr)
        data = json.loads((output / 'maps/test.epe.json').read_text())
        assert data['props'][str(prop_hash)] == 42

        blocked = root / 'blocked'
        (blocked / 'maps/test.epe.json').mkdir(parents=True)
        result = extract(blocked)
        assert result.returncode != 0, (result.stdout, result.stderr)
        assert 'Failed to open RTPC JSON output' in result.stderr
    print('RTPC JSON output tests passed')


if __name__ == '__main__':
    main()
