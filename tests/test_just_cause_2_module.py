#!/usr/bin/env python3
"""Exercise JC2 TAB/ARC extraction, distinct base/patch/DLC archives, and root probing."""
import argparse
import json
from pathlib import Path
import sqlite3
import struct
import subprocess
import tempfile
import zlib


def archive(folder, name, entries):
    folder.mkdir(parents=True, exist_ok=True)
    tab = bytearray(struct.pack('<I', 2048))
    arc = bytearray()
    for asset_hash, payload in entries:
        offset = len(arc)
        assert offset % 2048 == 0
        tab.extend(struct.pack('<III', asset_hash, offset, len(payload)))
        arc.extend(payload)
        arc.extend(b'\0' * (-len(arc) % 2048))
    (folder / f'{name}.tab').write_bytes(tab)
    (folder / f'{name}.arc').write_bytes(arc)



def pcbb_section(properties):
    data = bytearray()

    def siblings(items):
        data.extend(b'\0' * (-len(data) % 16))
        offsets = []
        for _ in items:
            offsets.append(len(data))
            data.extend(b'\xdd' * 32)
        for index, (name_hash, kind, value) in enumerate(items):
            offset = offsets[index]
            following = offsets[index + 1] if index + 1 < len(offsets) else 0xFFFFFFFF
            struct.pack_into('<4I', data, offset, name_hash, 1 if kind == 'group' else 2,
                             offset + 16, following)
            if kind == 'group':
                struct.pack_into('<I', data, offset + 16, siblings(value))
            else:
                value_kind = {'u32': 1, 'string': 3, 'matrix': 8}[kind]
                struct.pack_into('<I', data, offset + 16, value_kind)
                if kind == 'u32':
                    struct.pack_into('<I', data, offset + 20, value)
                else:
                    data.extend(b'\0' * (-len(data) % 4))
                    struct.pack_into('<I', data, offset + 20, len(data))
                    data.extend(value.encode() + b'\0' if kind == 'string' else struct.pack('<16f', *value))
        return offsets[0]

    siblings(properties)
    data.extend(b'\0' * (-len(data) % 16))
    return struct.pack('<4sI', b'PCBB', len(data)) + data


class_key = 0x1473B179
name_key = 0xD31AB684
world_key = 1822872761
identity = (1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    args = parser.parse_args()
    binary = args.binary.resolve()
    with tempfile.TemporaryDirectory(prefix='apex-jc2-') as temp:
        root = Path(temp) / 'Just Cause 2'
        base = root / 'archives_win32'
        dlc = root / 'DLC'
        root.mkdir()
        (root / 'JustCause2.exe').write_bytes(b'MZ')
        archive(base, 'pc0', [(0x11223345, zlib.compress(b'old compressed data'))])
        archive(base, 'pc4', [(0x11223344, zlib.compress(b'new compressed data'))])
        child_world = identity[:12] + (3.0, 4.0, 5.0, 1.0)
        entity = pcbb_section([
            (class_key, 'string', 'CGeometryObject'),
            (name_key, 'string', 'fixture.root'),
            (0x01020304, 'group', [
                (class_key, 'string', 'CSimpleRigidObject'),
                (name_key, 'string', 'fixture.child'),
                (world_key, 'matrix', child_world),
                (0x23456789, 'group', [(name_key, 'string', 'fixture.grandchild')]),
                (0x33333333, 'u32', 0xF907D551),
            ]),
        ]) + pcbb_section([
            (class_key, 'string', 'CEventTrigger'),
            (name_key, 'string', 'fixture.second'),
        ])
        archive(base, 'pc1', [(0x2468ACE0, entity)])
        archive(dlc, 'pc_60', [(0xCAFEBABE, b'DLC stored data')])
        # These old-game archives are not inside the newer engine's initial folder.
        archive(root / 'initial', 'ignored', [(0x11223344, b'wrong folder')])
        db = Path(temp) / 'hashes.db'
        with sqlite3.connect(db) as connection:
            connection.executescript("""
                CREATE TABLE kv (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,
                    v TEXT NOT NULL, PRIMARY KEY(lookup3,murmur));
                CREATE TABLE files (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,
                    name TEXT, size INTEGER NOT NULL, parent INTEGER NOT NULL DEFAULT 0,
                    PRIMARY KEY(lookup3,murmur));
                PRAGMA user_version=1;
            """)
        output = Path(temp) / 'output'

        def run(*arguments, success=True):
            result = subprocess.run([str(binary), *map(str, arguments)], cwd=temp,
                                    capture_output=True, text=True)
            assert (result.returncode == 0) == success, (result.returncode, result.stdout, result.stderr)
            return result.stdout + result.stderr

        for path in (root, base, str(base) + '/'):
            assert 'supported [100]' in run('modules', path, '--module', 'just-cause-2')
        run('extract', base, '0x11223344', '-d', db, '-o', output)
        assert (output / 'file_11223344.bin').read_bytes() == b'new compressed data'
        run('extract', root, '0x11223345', '-d', db, '-o', output)
        assert (output / 'file_11223345.bin').read_bytes() == b'old compressed data'
        run('extract', root, '0xCAFEBABE', '-r', '-d', db, '-o', output)
        assert (output / 'CAFEBABE.bin').read_bytes() == b'DLC stored data'
        run('extract', root, '0x2468ACE0', '-d', db, '-o', output)
        metadata = json.loads((output / 'path_2468ACE0.json').read_text())
        assert len(metadata) == 2
        assert metadata[0]['_class'] == 'CGeometryObject'
        assert metadata[0]['16909060']['name'] == 'fixture.child'
        assert metadata[0]['16909060']['858993459'] == 0xF907D551
        scene = json.loads((output / '2468ACE0.gltf').read_text())
        nodes = scene['nodes']
        scene_root = nodes[scene['scenes'][0]['nodes'][0]]
        assert scene_root['name'] == 'epe_root'
        sections = [nodes[index] for index in scene_root['children']]
        assert [node['name'] for node in sections] == ['fixture.root', 'fixture.second']
        child = nodes[sections[0]['children'][0]]
        assert child['name'] == 'fixture.child'
        assert child['matrix'][12:15] == [3.0, 4.0, 5.0]
        assert child['extras']['_class'] == 'CSimpleRigidObject'
        grandchild = nodes[child['children'][0]]
        assert grandchild['name'] == 'fixture.grandchild'
        assert 'Asset not found' in run('extract', root, '0x12345678', '-d', db, '-o', output, success=False)
    print('Just Cause 2 TAB module tests passed')


if __name__ == '__main__':
    main()
