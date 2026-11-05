#!/usr/bin/env python3
"""Exercise both 2048-aligned TAB layouts through the shared archive facade."""
import sqlite3
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib


def main(binary):
    with tempfile.TemporaryDirectory(prefix='apex-tab-v1-dispatch-') as temp:
        root = Path(temp) / 'Just Cause 2'
        archive = root / 'archives_win32'
        archive.mkdir(parents=True)
        (root / 'DLC').mkdir()
        (root / 'JustCause2.exe').write_bytes(b'MZ')
        old = b'JC2 zlib stream ' * 19
        packed_old = zlib.compress(old)
        (archive / 'pc0.tab').write_bytes(struct.pack('<4I', 2048, 0x11223344, 0, len(packed_old)))
        (archive / 'pc0.arc').write_bytes(packed_old)

        first, second = b'raw deflate block ' * 15, b'second raw block ' * 23
        packed_first = zlib.compress(first, wbits=-15)
        packed_second = zlib.compress(second, wbits=-15)
        stored = b'V1 stored asset'
        (archive / 'pc1.tab').write_bytes(
            struct.pack('<II', 2048, 1) +
            struct.pack('<II4I', 0x55667788, 2, 0, 0, len(first), len(packed_first)) +
            struct.pack('<4I', 0x55667788, 0, len(packed_first) + len(packed_second), len(first) + len(second)) +
            struct.pack('<4I', 0x99AABBCC, 2048, len(stored), len(stored)))
        (archive / 'pc1.arc').write_bytes(
            packed_first + packed_second + b'\0' * (2048 - len(packed_first) - len(packed_second)) + stored)

        db = Path(temp) / 'hashes.db'
        with sqlite3.connect(db) as connection:
            connection.executescript(
                'CREATE TABLE kv (lookup3 INTEGER, murmur INTEGER, v TEXT);'
                'CREATE TABLE files (lookup3 INTEGER, murmur INTEGER, name TEXT, size INTEGER, parent INTEGER);')
        output = Path(temp) / 'output'
        for hash_value, expected in ((0x11223344, old), (0x55667788, first + second),
                                     (0x99AABBCC, stored)):
            command = [str(binary), 'extract', str(root), f'0x{hash_value:08X}', '--module',
                       'just-cause-2', '--raw', '-d', str(db), '-o', str(output)]
            result = subprocess.run(command, cwd=temp, capture_output=True, text=True)
            assert result.returncode == 0, (command, result.stdout, result.stderr)
            assert (output / f'{hash_value:08X}.bin').read_bytes() == expected
    print('TAB v1 and JC2 layout dispatch passed')


if __name__ == '__main__':
    main(Path(sys.argv[1]).resolve())
