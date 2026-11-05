#!/usr/bin/env python3
"""Snapshot a WAL database from an absolute path and stage its tar archive."""
import argparse
from pathlib import Path
import sqlite3
import subprocess
import sys
import tarfile
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('cmake', type=Path)
    cmake = parser.parse_args().cmake.resolve()
    packager = Path(__file__).resolve().parents[1] / 'cmake' / 'package_databases.py'

    with tempfile.TemporaryDirectory(prefix='apex package databases-') as directory:
        root = Path(directory)
        source = root / 'fixture.db'
        with sqlite3.connect(source) as database:
            database.execute('PRAGMA journal_mode=WAL')
            database.execute('CREATE TABLE entries (name TEXT PRIMARY KEY)')
            database.execute('INSERT INTO entries VALUES (?)', ('committed WAL row',))
            database.commit()
            subprocess.run([sys.executable, str(packager), str(root)], check=True)

        archive = root / 'fixture.db.tar.xz'
        with tarfile.open(archive, 'r:xz') as compressed:
            assert compressed.getnames() == ['fixture.db'], compressed.getnames()

        stage = root / 'stage'
        stage.mkdir()
        subprocess.run([str(cmake), '-E', 'tar', 'xJf', str(archive)], cwd=stage, check=True)
        with sqlite3.connect(stage / 'fixture.db') as extracted:
            assert extracted.execute('SELECT name FROM entries').fetchall() == [('committed WAL row',)]
    print('Portable database archive staging passed')


if __name__ == '__main__':
    main()
