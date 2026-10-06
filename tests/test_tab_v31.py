#!/usr/bin/env python3
"""Synthetic TAB 3.1 extraction and validation; no proprietary assets needed."""
import argparse
from pathlib import Path
import struct
import sqlite3
import subprocess
import tempfile
import zlib


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    args = parser.parse_args()
    binary = args.binary.resolve()
    with tempfile.TemporaryDirectory(prefix='apex-tab31-') as tmp:
        root = Path(tmp)
        initial = root / 'archives_win64/initial'
        initial.mkdir(parents=True)
        (root / 'RAGE2.exe').write_bytes(b'MZ')
        tab = initial / 'game0.tab'
        arc = tab.with_suffix('.arc')
        output = root / 'out'
        database = root / 'hashes.db'
        with sqlite3.connect(database) as connection:
            connection.executescript(
                'CREATE TABLE kv (lookup3 INTEGER, murmur INTEGER, v TEXT);'
                'CREATE TABLE files (lookup3 INTEGER, murmur INTEGER, name TEXT, size INTEGER, parent INTEGER);')


        def archive(entries, blocks, data):
            header = struct.pack('<4sHH6I', b'TAB\0', 3, 1, 4096, len(entries), len(blocks), 0, 0, 0)
            tab.write_bytes(header + b''.join(struct.pack('<II', *b) for b in blocks) +
                            b''.join(struct.pack('<QIIIHBB', *e) for e in entries))
            arc.write_bytes(data)

        def extract(asset, success=True):
            result = subprocess.run([str(binary), 'extract', str(root), asset, '-r', '-d', str(database),
                                     '-o', str(output)],
                                    capture_output=True, text=True)
            text = result.stdout + result.stderr
            assert (result.returncode == 0) == success, (asset, result.returncode, text)
            if success:
                hash_value = int(asset, 16) if asset.startswith('0x') else next(
                    h for path, h in paths if path == asset)
                return output / f'{hash_value:08X}.bin'
            return output / asset

        sentinel = (0xffffffff, 0xffffffff)
        # The message's three Murmur3 vectors validate full-width path lookup.
        paths = [('text/master_eng.stringlookup', 0x8453EE3581F31F39),
                 ('ai/tiles/43_37.navmeshc', 0xE73A9076A24B7E8D),
                 ('terrain/heatwave/patches/width_8/patch_11_00_00.streampatch', 0x95C8093796E2DD75)]
        raw = b'raw archive payload'
        entries = [(h, 0, len(raw), len(raw), 0, 0, 0) for _, h in paths]
        archive(entries, [sentinel, sentinel], raw)
        for path, _ in paths:
            assert extract(path).read_bytes() == raw
        extract('0xdeadbeef', False)
        extract('0x123invalid', False)

        data = b'zlib payload' * 100
        packed = zlib.compress(data)
        archive([(1, 7, len(packed), len(data), 0, 1, 0)], [sentinel], b'padding' + packed)
        assert extract('0x1').read_bytes() == data

        # Independently compressed streams packed without alignment, first at index 1.
        second = b'second block' * 40
        packed2 = zlib.compress(second)
        archive([(2, 0, len(packed) + len(packed2), len(data) + len(second), 1, 1, 1)],
                [sentinel, (len(packed), len(data)), (len(packed2), len(second))], packed + packed2)
        assert extract('0x2').read_bytes() == data + second

        # Second Extinction uses Zstd (codec 3) in TAB 3.1 archives.
        zstd_data = b'zstd payload' * 100
        zstd_frame = bytes.fromhex('28b52ffd60b0039d0000607a737464207061796c6f61640100a1fc2f49')
        archive([(7, 0, len(zstd_frame), len(zstd_data), 0, 3, 0)], [sentinel], zstd_frame)
        assert extract('0x7').read_bytes() == zstd_data
        archive([(8, 0, len(zstd_frame) + len(second), len(zstd_data) + len(second), 1, 3, 1)],
                [sentinel, (len(zstd_frame), len(zstd_data)), (len(second), len(second))],
                zstd_frame + second)
        assert extract('0x8').read_bytes() == zstd_data + second

        # Valid Oodle stream with an uncompressed quantum exercises the native decoder.
        oodle = b'\xcc\x06' + data
        archive([(3, 0, len(oodle), len(data), 0, 4, 0)], [sentinel], oodle)
        assert extract('0x3').read_bytes() == data
        # Mixed Oodle and stored blocks; flags do not determine the codec.
        archive([(4, 0, len(oodle) + len(second), len(data) + len(second), 1, 4, 1)],
                [sentinel, (len(oodle), len(data)), (len(second), len(second))], oodle + second)
        assert extract('0x4').read_bytes() == data + second
        archive([(5, 0, 0, 0, 0, 0, 0)], [sentinel, sentinel], b'')
        assert extract('0x5').read_bytes() == b''

        cases = [
            ([(6, 4, 2, 2, 0, 0, 0)], [sentinel], b'aa'),
            ([(6, 0, 2, 4, 3, 1, 0)], [sentinel], b'aa'),
            ([(6, 0, 2, 4, 1, 1, 0)], [sentinel, sentinel], b'aa'),
            ([(6, 0, 2, 4, 1, 1, 0)], [sentinel, (3, 4)], b'aa'),
            ([(6, 0, 2, 4, 0, 1, 0)], [sentinel], b'aa'),
            ([(6, 0, 2, 4, 0, 4, 0)], [sentinel], b'aa'),
            ([(6, 0, 2, 4, 0, 3, 0)], [sentinel], b'aa'),
            ([(6, 0, 2, 2, 0, 9, 0)], [sentinel], b'aa'),
        ]
        for entries, blocks, payload in cases:
            archive(entries, blocks, payload)
            extract('0x6', False)
        archive([(6, 0, 2, 2, 0, 0, 0)], [sentinel], b'aa')
        valid = tab.read_bytes()
        tab.write_bytes(valid[:-1])
        extract('0x6', False)
    print('TAB 3.1 extraction tests passed')


if __name__ == '__main__':
    main()
