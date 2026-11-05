#!/usr/bin/env python3
"""Decode Mad Max's own generated ADF type through the loaded game module."""
import argparse
import json
from pathlib import Path
import sqlite3
import struct
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    parser.add_argument('module', type=Path)
    args = parser.parse_args()
    binary, module = args.binary.resolve(), args.module.resolve()

    with tempfile.TemporaryDirectory(prefix='apex-mad-max-adf-') as temp:
        root = Path(temp) / 'Mad Max'
        (root / 'bin').mkdir(parents=True)
        (root / 'bin/steam_appid.txt').write_text('234140\n')
        archives = root / 'share/data/archives_win64'
        archives.mkdir(parents=True)
        # Small ADF root type ListDirectoryDesc: one vector of one string.
        adf = (struct.pack('<4sI', b'\0FDA', 0x6746FECA)
               + struct.pack('<IIII', 16, 0, 1, 0)
               + struct.pack('<II', 24, 0) + b'Mad Max\0')
        unknown = struct.pack('<4sI', b'\0FDA', 0xDEAD1234) + bytes(16)
        records = [(0xDEADBEEF, adf), (0xABCDEF01, b'not an ADF'), (0xABCDEF02, unknown)]
        tab = bytearray(struct.pack('<II', 2048, 0))
        arc = bytearray()
        for key, payload in records:
            arc.extend(b'\0' * (-len(arc) % 2048))
            tab.extend(struct.pack('<IIII', key, len(arc), len(payload), len(payload)))
            arc.extend(payload)
        (archives / 'game0.tab').write_bytes(tab)
        (archives / 'game0.arc').write_bytes(arc)

        db = Path(temp) / 'hashes.db'
        with sqlite3.connect(db) as connection:
            connection.executescript("""
                CREATE TABLE kv (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,
                    v TEXT NOT NULL, PRIMARY KEY(lookup3,murmur));
                CREATE TABLE files (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,
                    name TEXT, size INTEGER NOT NULL, parent INTEGER NOT NULL DEFAULT 0,
                    PRIMARY KEY(lookup3,murmur));
                PRAGMA user_version=1;
            """)

        output = Path(temp) / 'out'

        def extract(key):
            return subprocess.run([str(binary), 'extract', str(root), f'0x{key:08X}',
                                   '--module', str(module), '-d', str(db), '-o', str(output)],
                                  cwd=temp, capture_output=True, text=True)

        result = extract(0xDEADBEEF)
        assert result.returncode == 0, (result.stdout, result.stderr)
        assert json.loads((output / 'DEADBEEF.json').read_text()) == [
            {'type_hash': '0x6746FECA', 'value': {'Files': ['Mad Max']}}]
        for key in (0xABCDEF01, 0xABCDEF02):
            result = extract(key)
            assert result.returncode != 0, (key, result.stdout, result.stderr)
            assert not (output / f'{key:08X}.json').exists()

    print('Mad Max ADF module tests passed')


if __name__ == '__main__':
    main()
