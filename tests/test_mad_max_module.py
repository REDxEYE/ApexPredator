#!/usr/bin/env python3
"""Exercise Mad Max TAB/ARC decoding and native mount precedence."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib


def archive(folder, number, records):
    folder.mkdir(parents=True, exist_ok=True)
    tab = bytearray(struct.pack('<II', 2048, sum(len(parts) > 1 for _, parts in records)))
    arc = bytearray()
    for hash_value, parts in records:
        if len(parts) > 1:
            tab.extend(struct.pack('<II', hash_value, len(parts)))
            packed = unpacked = 0
            for payload, stored in parts:
                tab.extend(struct.pack('<II', unpacked, packed))
                unpacked += len(payload)
                packed += len(payload) if stored else len(zlib.compress(payload, wbits=-15))
    for hash_value, parts in records:
        arc.extend(b'\0' * (-len(arc) % 2048))
        offset = len(arc)
        content = b''.join(payload if stored else zlib.compress(payload, wbits=-15)
                           for payload, stored in parts)
        arc.extend(content)
        tab.extend(struct.pack('<IIII', hash_value, offset, len(content),
                               sum(len(payload) for payload, _ in parts)))
    (folder / f'game{number}.tab').write_bytes(tab)
    (folder / f'game{number}.arc').write_bytes(arc)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    parser.add_argument('module', type=Path)
    args = parser.parse_args()
    binary, module = args.binary.resolve(), args.module.resolve()
    with tempfile.TemporaryDirectory(prefix='apex-mad-max-') as temp:
        root = Path(temp) / 'Mad Max'
        data = root / 'share/data'
        (root / 'bin').mkdir(parents=True)
        (root / 'bin/steam_appid.txt').write_text('234140\n')
        base = data / 'archives_win64'
        archive(base, 0, [(0xC0AB771A, [(b'basename lookup\n', True)]),
                          (0x12345678, [(b'compressed first ' * 20, False),
                                        (b'and stored second', True)]),
                          (0xBBBBBBBB, [(b'base', True)]),
                          (0xCCCCCCCC, [(b'base', True)])])
        archive(base, 9, [(0xAAAAAAAA, [(b'game9', True)])])
        archive(base, 10, [(0xAAAAAAAA, [(b'game10', True)])])
        archive(data / 'patch_win64', 0,
                [(0xBBBBBBBB, [(b'patch', True)]), (0xCCCCCCCC, [(b'patch', True)])])
        archive(data / 'feral_overrides', 0, [(0xCCCCCCCC, [(b'feral', True)])])
        archive(data / 'dlc_win64/ripper', 0, [(0xDDDDDDDD, [(b'dlc', True)])])

        def run(*options, success=True):
            result = subprocess.run([str(binary), *map(str, options), '--module', str(module)],
                                    cwd=temp, capture_output=True, text=True)
            assert (result.returncode == 0) == success, (result.returncode, result.stdout, result.stderr)
            return result.stdout + result.stderr

        for location in (root, data, base, str(base) + '/'):
            assert 'supported [100]' in run('modules', location)
        output = Path(temp) / 'out'
        for hash_value, expected in [('folder\\asset.txt', b'basename lookup\n'),
                                     ('0x12345678', b'compressed first ' * 20 + b'and stored second'),
                                     ('0xAAAAAAAA', b'game10'), ('0xBBBBBBBB', b'patch'),
                                     ('0xCCCCCCCC', b'feral'), ('0xDDDDDDDD', b'dlc')]:
            run('extract', root, hash_value, '--raw', '-o', output)
            filename = 'C0AB771A.bin' if '\\' in hash_value else f'{int(hash_value, 16):08X}.bin'
            assert (output / filename).read_bytes() == expected
        assert 'pass --raw' in run('extract', root, '0x12345678', '-o', output, success=False)
        assert 'Asset not found' in run('extract', root, '0x00000001', '--raw', '-o', output, success=False)
        windows = Path(temp) / 'Windows Mad Max'
        windows.mkdir()
        (windows / 'MadMax.exe').write_bytes(b'MZ')
        archive(windows / 'archives_win64', 0,
                [(0x12345678, [(b'windows compressed ' * 10, False)]),
                 (0xBBBBBBBB, [(b'windows base', True)])])
        archive(windows / 'patch_win64', 0, [(0xBBBBBBBB, [(b'windows patch', True)])])
        archive(windows / 'dlc_win64/ripper', 0, [(0xDDDDDDDD, [(b'windows dlc', True)])])
        for location in (windows, windows / 'archives_win64'):
            assert 'supported [100]' in run('modules', location)
        for hash_value, expected in [('0x12345678', b'windows compressed ' * 10),
                                     ('0xBBBBBBBB', b'windows patch'),
                                     ('0xDDDDDDDD', b'windows dlc')]:
            run('extract', windows, hash_value, '--raw', '-o', output)
            assert (output / f'{int(hash_value, 16):08X}.bin').read_bytes() == expected
        (root / 'bin/steam_appid.txt').write_text('548570\n')
        assert 'unsupported [0]' in run('modules', root)
    print('Mad Max archive extraction tests passed')


if __name__ == '__main__':
    main()
