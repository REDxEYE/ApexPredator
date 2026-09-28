#!/usr/bin/env python3
"""Synthetic TAB 3.1 extraction and validation; no proprietary assets needed."""
import argparse
from pathlib import Path
import struct
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

        def archive(entries, blocks, data):
            header = struct.pack('<4sHH6I', b'TAB\0', 3, 1, 4096, len(entries), len(blocks), 0, 0, 0)
            tab.write_bytes(header + b''.join(struct.pack('<II', *b) for b in blocks) +
                            b''.join(struct.pack('<QIIIHBB', *e) for e in entries))
            arc.write_bytes(data)

        def extract(asset, success=True, expected_error=None):
            result = subprocess.run([str(binary), 'extract', str(root), asset, '-r', '-o', str(output)],
                                    capture_output=True, text=True)
            text = result.stdout + result.stderr
            assert (result.returncode == 0) == success, (asset, result.returncode, text)
            if expected_error:
                assert expected_error in text, text
            return output / (asset + '.bin' if asset.startswith('0x') else asset)

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
        extract('0xdeadbeef', False, 'Asset not found')
        extract('../escape', False, "must not contain '..'")
        extract('0x123invalid', False, 'Invalid 64-bit asset hash')

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
            ([(6, 4, 2, 2, 0, 0, 0)], [sentinel], b'aa', 'exceeds ARC bounds'),
            ([(6, 0, 2, 4, 3, 1, 0)], [sentinel], b'aa', 'block index out of range'),
            ([(6, 0, 2, 4, 1, 1, 0)], [sentinel, sentinel], b'aa', 'sentinel'),
            ([(6, 0, 2, 4, 1, 1, 0)], [sentinel, (3, 4)], b'aa', 'block sizes'),
            ([(6, 0, 2, 4, 0, 1, 0)], [sentinel], b'aa', 'zlib decompression failed'),
            ([(6, 0, 2, 4, 0, 4, 0)], [sentinel], b'aa', 'Oodle decompression failed'),
            ([(6, 0, 2, 2, 0, 9, 0)], [sentinel], b'aa', 'compression type'),
        ]
        for entries, blocks, payload, error in cases:
            archive(entries, blocks, payload)
            extract('0x6', False, error)
        archive([(6, 0, 2, 2, 0, 0, 0)], [sentinel], b'aa')
        valid = tab.read_bytes()
        tab.write_bytes(valid[:-1])
        extract('0x6', False, 'table sizes')
        tab.write_bytes(valid)
        # Nested supplemental language archives participate and override base assets.
        nested = root / 'archives_win64/supplemental/languages/eng'
        nested.mkdir(parents=True)
        (nested / 'game0.tab').write_bytes(valid)
        (nested / 'game0.arc').write_bytes(b'bb')
        assert extract('0x6').read_bytes() == b'bb'
    print('TAB 3.1 extraction tests passed')


if __name__ == '__main__':
    main()
