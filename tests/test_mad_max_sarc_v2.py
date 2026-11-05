#!/usr/bin/env python3
"""Resolve a null-padded SARC v2 member through Mad Max's TAB/ARC and asset DB."""
import argparse
from pathlib import Path
import sqlite3
import struct
import subprocess
import tempfile


MEMBER = 'animations/skeletons/vehicles/gastown001light/gastown001light.bsk'
MEMBER_HASH = 0xB3D63E83
PARENT_HASH = 0xA616F002


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    parser.add_argument('module', type=Path)
    args = parser.parse_args()
    binary, module = args.binary.resolve(), args.module.resolve()

    with tempfile.TemporaryDirectory(prefix='apex-mad-max-sarc-') as temp:
        root = Path(temp) / 'Mad Max'
        archives = root / 'share/data/archives_win64'
        archives.mkdir(parents=True)
        (root / 'bin').mkdir()
        (root / 'bin/steam_appid.txt').write_text('234140\n')

        # v2 records store a padded path. Its lookup key must exclude the NUL padding.
        name = MEMBER.encode() + b'\0\0'
        payload = b'skeleton member payload\n'
        directory_size = 4 + len(name) + 8
        sarc = (struct.pack('<I4sII', 4, b'SARC', 2, directory_size)
                + struct.pack('<I', len(name)) + name
                + struct.pack('<II', 16 + directory_size, len(payload)) + payload)
        (archives / 'game0.tab').write_bytes(struct.pack('<II', 2048, 0)
                                             + struct.pack('<IIII', PARENT_HASH, 0, len(sarc), len(sarc)))
        (archives / 'game0.arc').write_bytes(sarc)

        db_path = Path(temp) / 'hashes.db'
        with sqlite3.connect(db_path) as db:
            db.executescript('''
                CREATE TABLE kv (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,
                    v TEXT NOT NULL, PRIMARY KEY(lookup3,murmur));
                CREATE TABLE files (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,
                    name TEXT, size INTEGER NOT NULL, parent INTEGER NOT NULL DEFAULT 0,
                    PRIMARY KEY(lookup3,murmur));
                PRAGMA user_version=1;
            ''')
            db.execute('INSERT INTO files VALUES (?,0,?,?,?)',
                       (MEMBER_HASH, MEMBER, len(payload), PARENT_HASH))
            db.execute('INSERT INTO kv VALUES (?,0,?)', (MEMBER_HASH, MEMBER))

        output = Path(temp) / 'out'
        result = subprocess.run([str(binary), 'extract', str(root), f'0x{MEMBER_HASH:08X}',
                                 '--module', str(module), '--raw', '-d', str(db_path), '-o', str(output)],
                                cwd=temp, capture_output=True, text=True)
        assert result.returncode == 0, (result.stdout, result.stderr)
        assert (output / MEMBER).read_bytes() == payload

    print('Mad Max SARC v2 padded-name lookup passed')


if __name__ == '__main__':
    main()
