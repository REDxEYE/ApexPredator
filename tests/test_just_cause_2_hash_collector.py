#!/usr/bin/env python3
"""Verify the shared JC2 collector's default corpus, legacy hashes, and DLC precedence."""
import argparse
from pathlib import Path
import sqlite3
import subprocess
import tempfile

from test_just_cause_2_module import archive


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('collector', type=Path)
    collector = parser.parse_args().collector.resolve()
    with tempfile.TemporaryDirectory(prefix='apex-jc2-hashes-') as temp:
        root = Path(temp) / 'Just Cause 2'
        root.mkdir()
        (root / 'JustCause2.exe').write_bytes(b'MZ')
        known = 2035976115  # Lookup3('AnimationSet')
        archive(root / 'archives_win32', 'pc0', [(known, b'old archive payload')])
        archive(root / 'archives_win32', 'pc4', [(known, b'newer archive payload')])
        override = b'DLC override payload'
        archive(root / 'DLC', 'pc_60', [(known, override)])

        corpus = Path(temp) / 'strings/just_cause_2/just_cause_2_exe_strings.txt'
        corpus.parent.mkdir(parents=True)
        corpus.write_text('AnimationSet\nnot_installed.file\n')
        database = Path(temp) / 'hashes.db'
        with sqlite3.connect(database) as db:
            db.executescript('CREATE TABLE kv(k INTEGER PRIMARY KEY,v TEXT NOT NULL) WITHOUT ROWID;'
                             'CREATE TABLE files(hash INTEGER PRIMARY KEY,name TEXT,size INTEGER NOT NULL,parent INTEGER NOT NULL DEFAULT 0) WITHOUT ROWID;')
            db.execute('INSERT INTO kv VALUES(?,?)', (known, 'AnimationSet'))
            db.execute('INSERT INTO files VALUES(?,?,?,?)', (known, 'AnimationSet', 3, 0))

        work = Path(temp) / 'bin'
        work.mkdir()
        for _ in range(2):
            result = subprocess.run([str(collector), str(root / 'archives_win32') + '/'],
                                    cwd=work, capture_output=True, text=True)
            assert result.returncode == 0, result.stdout + result.stderr
        with sqlite3.connect(database) as db:
            assert db.execute('SELECT lookup3,murmur,v FROM kv ORDER BY lookup3').fetchall() == [
                (263034721, -2096487358734672743, 'not_installed.file'),
                (known, -7094997017260814282, 'AnimationSet')]
            assert db.execute('SELECT lookup3,murmur,name,size,parent FROM files_just_cause_2').fetchall() == [
                (known, -7094997017260814282, 'AnimationSet', len(override), 0)]
    print('Just Cause 2 collector migration and DLC precedence passed')


if __name__ == '__main__':
    main()
