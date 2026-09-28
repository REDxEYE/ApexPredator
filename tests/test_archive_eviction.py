#!/usr/bin/env python3
"""Exercise collector callbacks that lazily mount more than 32 child archives."""
import pathlib
import sqlite3
import struct
import subprocess
import sys
import tempfile
import zlib

executable = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='apex-archive-eviction-') as tmp:
    root = pathlib.Path(tmp)
    work = root / 'bin'
    work.mkdir()
    initial = root / 'game/archives_win64/initial'
    initial.mkdir(parents=True)
    database = root / 'hashes.db'
    # Empty, valid SARC wrapped in AAF. Parent lookup mounts it while the
    # collector is still enumerating the TAB containing the triggering file.
    sarc = struct.pack('<I4sIII', 1, b'SARC', 3, 4, 0)
    compressor = zlib.compressobj(wbits=-15)
    compressed = compressor.compress(sarc) + compressor.flush()
    aaf = struct.pack('<4sI28sIII', b'AAF\0', 1, b'', len(sarc), len(sarc), 1)
    aaf += struct.pack('<III4s', len(compressed), len(sarc), 16 + len(compressed), b'EWAM')
    aaf += compressed
    expected = {f'ArchiveEvictionString{i}' for i in range(40)}
    with sqlite3.connect(database) as db:
        db.executescript('CREATE TABLE kv(k INTEGER PRIMARY KEY,v TEXT NOT NULL) WITHOUT ROWID;'
                         'CREATE TABLE files(hash INTEGER PRIMARY KEY,name TEXT,size INTEGER NOT NULL,parent INTEGER NOT NULL DEFAULT 0) WITHOUT ROWID;')
        for i in range(40):
            child, parent = 1000 + i, 2000 + i
            payload = bytearray(36)
            payload[:20] = struct.pack('<4sIIIHH', b'RTPC', 3, 1, 20, 1, 0)
            payload[20:29] = struct.pack('<IIB', 1, 36, 3)
            payload += f'ArchiveEvictionString{i}\0'.encode()
            (initial / f'game{i}.arc').write_bytes(payload + aaf)
            tab = struct.pack('<4sHHI', b'TAB\0', 2, 1, 2048)
            tab += struct.pack('<III', child, 0, len(payload))
            tab += struct.pack('<III', parent, len(payload), len(aaf))
            (initial / f'game{i}.tab').write_bytes(tab)
            db.execute('INSERT INTO files VALUES(?,?,?,?)', (child, None, len(payload), parent))
    result = subprocess.run([executable, str(root / 'game')], cwd=work,
                            capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'Failed to read file' not in result.stdout + result.stderr, result.stdout + result.stderr
    with sqlite3.connect(database) as db:
        collected = {row[0] for row in db.execute("SELECT v FROM kv WHERE v LIKE 'ArchiveEvictionString%'")}
    assert collected == expected, f'Missing collector output: {expected - collected}'
print('Generation Zero collector passed 40-archive eviction regression')
