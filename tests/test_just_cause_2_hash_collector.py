#!/usr/bin/env python3
"""Verify JC2 hash collection, archive precedence, and known-path migration."""
import argparse
from pathlib import Path
import sqlite3
import subprocess
import tempfile

from test_just_cause_2_module import archive


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('collector', type=Path)
    args = parser.parse_args()
    collector = args.collector.resolve()
    with tempfile.TemporaryDirectory(prefix='apex-jc2-hashes-') as temp:
        root = Path(temp) / 'Just Cause 2'
        root.mkdir()
        (root / 'JustCause2.exe').write_bytes(b'MZ')
        known = 2035976115  # lookup3('AnimationSet'), anchored in test_hash_collectors.py.
        unknown = 0x12345678
        archive(root / 'archives_win32', 'pc0', [(known, b'old'), (unknown, b'base')])
        archive(root / 'archives_win32', 'pc4', [(known, b'newer')])
        archive(root / 'DLC', 'pc_60', [(unknown, b'DLC override')])
        candidates = Path(temp) / 'paths.txt'
        candidates.write_text('AnimationSet\nnot_installed.file\n')
        database = Path(temp) / 'jc2.db'
        # A JC2 legacy database must keep its 32-bit keys when migrated.
        with sqlite3.connect(database) as db:
            db.executescript('CREATE TABLE kv(k INTEGER PRIMARY KEY,v TEXT NOT NULL) WITHOUT ROWID;'
                             'CREATE TABLE files(hash INTEGER PRIMARY KEY,name TEXT,size INTEGER NOT NULL,parent INTEGER NOT NULL DEFAULT 0) WITHOUT ROWID;')
            db.execute('INSERT INTO kv VALUES(?,?)', (known, 'AnimationSet'))
            db.execute('INSERT INTO files VALUES(?,?,?,?)', (known, 'AnimationSet', 3, 0))

        for _ in range(2):
            result = subprocess.run([str(collector), str(root / 'archives_win32') + '/',
                                     str(database), str(candidates)], capture_output=True, text=True)
            assert result.returncode == 0, result.stdout + result.stderr
            assert '2 distinct TAB hashes' in result.stdout
        with sqlite3.connect(database) as db:
            files = db.execute('SELECT lookup3,murmur,name,size,parent FROM files ORDER BY lookup3').fetchall()
            assert files == [(unknown, 0, '', len(b'DLC override'), 0),
                             (known, 0, 'AnimationSet', len(b'newer'), 0)], files
            assert db.execute('SELECT lookup3,murmur,v FROM kv').fetchall() == [(known, 0, 'AnimationSet')]
        print('Just Cause 2 hash collector tests passed')


if __name__ == '__main__':
    main()
