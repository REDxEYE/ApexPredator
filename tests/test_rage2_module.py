#!/usr/bin/env python3
"""Verify Rage 2 registration, detection, and archive routing."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    parser.add_argument('module', type=Path)
    args = parser.parse_args()
    binary, module = args.binary.resolve(), args.module.resolve()
    with tempfile.TemporaryDirectory(prefix='apex-rage2-') as temp:
        folder = Path(temp)
        install = folder / 'RAGE 2'
        archives = install / 'archives_win64'
        initial = archives / 'initial'
        initial.mkdir(parents=True)
        executable = install / 'RAGE2.exe'
        executable.write_bytes(b'MZ')
        tab = initial / 'game0.tab'
        arc = initial / 'game0.arc'
        header = struct.pack('<4sHH6I', b'TAB\0', 3, 1, 4096, 0, 0, 0, 0, 0)
        tab.write_bytes(header)
        arc.write_bytes(b'')

        def run(*arguments, success=True):
            result = subprocess.run([str(binary), *map(str, arguments)], cwd=folder, capture_output=True, text=True)
            assert (result.returncode == 0) == success, (result.returncode, result.stdout, result.stderr)
            return result.stdout + result.stderr

        assert 'rage2 - Rage 2' in run('modules')
        for root in [install, archives, str(archives) + '/']:
            listing = run('modules', root, '--module', 'rage2')
            assert 'supported [100]' in listing and 'TAB 3.1' in listing
            assert 'unsupported [0]' in run('modules', root, '--module', 'generation-zero')
        run('extract', archives, 'asset.modelc', '-o', folder / 'output', success=False)
        assert not (folder / 'output').exists()
        run('extract', archives, '0x12345678', '--module', module, '-r', '-o', folder / 'output', success=False)
        assert not (folder / 'output').exists()

        # Archive-only copies can identify the game with its verified Steam app ID.
        executable.unlink()
        assert 'unsupported [0]' in run('modules', archives, '--module', 'rage2')
        marker = archives / 'steam_appid.txt'
        marker.write_text('548570\n')
        assert 'supported [100]' in run('modules', archives, '--module', 'rage2')
        marker.write_text('704270\n')
        assert 'unsupported [0]' in run('modules', archives, '--module', 'rage2')
        marker.write_text('548570\n')

        tab.write_bytes(header[:10])
        assert 'unsupported [0]' in run('modules', archives, '--module', 'rage2')
        wrong_version = bytearray(header); wrong_version[4] = 2
        tab.write_bytes(wrong_version)
        assert 'unsupported [0]' in run('modules', archives, '--module', 'rage2')
        tab.write_bytes(header)
        arc.unlink()
        assert 'unsupported [0]' in run('modules', archives, '--module', 'rage2')
        arc.write_bytes(b'')
        nested = archives / 'supplemental/languages/eng'
        nested.mkdir(parents=True)
        (nested / 'game0.tab').write_bytes(header)
        (nested / 'game0.arc').write_bytes(b'')
        assert 'supported [100]' in run('modules', archives, '--module', 'rage2')
        (nested / 'game0.arc').unlink()
        assert 'unsupported [0]' in run('modules', archives, '--module', 'rage2')
        # Explicit wrong-game selection must also fail before opening a session.
        assert 'does not support this root' in run('extract', archives, 'asset', '--module', 'generation-zero', success=False)
    print('Rage 2 module registration tests passed')


if __name__ == '__main__':
    main()
