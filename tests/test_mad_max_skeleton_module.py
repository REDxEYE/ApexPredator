#!/usr/bin/env python3
"""Decode a Havok 2013 skeleton with real section, array, and pointer fixups."""
import argparse
import json
from pathlib import Path
import sqlite3
import struct
import subprocess
import tempfile


SKELETON_HASH = 0xB3D63E83
SKELETON_NAME = 'animations/skeletons/vehicles/gastown001light/gastown001light.bsk'


def packfile():
    classes = bytearray()
    class_offsets = {}
    for name in ('hkRootLevelContainer', 'hkaAnimationContainer', 'hkaSkeleton'):
        class_offsets[name] = len(classes)
        classes.extend(name.encode() + b'\0')
    classes.extend(b'\xff' * (-len(classes) % 16))

    data = bytearray(0x270)

    def array(at, count):
        struct.pack_into('<II', data, at + 8, count, 0x80000000 | count)

    array(0x00, 1)      # hkRootLevelContainer.namedVariants
    array(0x50, 1)      # hkaAnimationContainer.skeletons
    struct.pack_into('<HH', data, 0xC8, 0, 1)  # hkaSkeleton reference count
    array(0xD8, 1)      # parentIndices
    array(0xE8, 1)      # bones
    array(0xF8, 1)      # referencePose
    for at in (0x108, 0x118, 0x128, 0x138):
        array(at, 0)    # referenceFloats, floatSlots, localFrames, partitions
    struct.pack_into('<h', data, 0x1C0, -1)
    data[0x1D8] = 0     # bone.lockTranslation
    struct.pack_into('<12f', data, 0x1E0,
                     1.0, 2.0, 3.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0)
    for at, value in ((0x1B0, b'Reference'), (0x220, b'hip'),
                      (0x230, b'hkaAnimationContainer'), (0x250, b'Animation Container')):
        data[at:at + len(value) + 1] = value + b'\0'

    fixups = [(0x00, 0x180), (0x50, 0x1A0), (0xD0, 0x1B0),
              (0xD8, 0x1C0), (0xE8, 0x1D0), (0xF8, 0x1E0),
              (0x180, 0x250), (0x188, 0x230), (0x190, 0x40),
              (0x1A0, 0xC0), (0x1D0, 0x220)]
    local = b''.join(struct.pack('<II', *fixup) for fixup in fixups)
    local += b'\xff' * (-len(local) % 16)
    virtual = b''.join(struct.pack('<III', at, 0, class_offsets[name])
                       for at, name in ((0, 'hkRootLevelContainer'),
                                        (0x40, 'hkaAnimationContainer'),
                                        (0xC0, 'hkaSkeleton')))
    virtual += b'\xff' * (-len(virtual) % 16)
    data.extend(local)
    data.extend(virtual)

    class_start = 256
    data_start = class_start + len(classes)
    header = bytearray(64)
    header[:8] = bytes.fromhex('57 e0 e0 57 10 c0 c0 10')
    struct.pack_into('<II', header, 12, 11, 0x01000108)
    struct.pack_into('<IIII', header, 20, 3, 2, 0, 0)
    struct.pack_into('<I', header, 36, class_offsets['hkRootLevelContainer'])
    header[40:55] = b'hk_2013.2.0-r1\0'
    header[62:64] = b'\xff\xff'

    def section(name, start, local_at, global_at, virtual_at, end):
        tag = name.encode() + b'\0'
        section_header = bytearray(64)
        section_header[:len(tag)] = tag
        section_header[19] = 0xff
        struct.pack_into('<7I', section_header, 20, start, local_at, global_at,
                         virtual_at, end, end, end)
        section_header[48:] = b'\xff' * 16
        return section_header

    data_size = 0x270
    section_headers = (
        section('__classnames__', class_start, len(classes), len(classes), len(classes), len(classes))
        + section('__types__', data_start, 0, 0, 0, 0)
        + section('__data__', data_start, data_size, data_size + len(local),
                  data_size + len(local), len(data))
    )
    return bytes(header + section_headers + classes + data)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    parser.add_argument('module', type=Path)
    args = parser.parse_args()
    binary, module = args.binary.resolve(), args.module.resolve()
    skeleton = packfile()
    with tempfile.TemporaryDirectory(prefix='apex-mad-max-skeleton-') as temp:
        root = Path(temp) / 'Mad Max'
        archives = root / 'share/data/archives_win64'
        archives.mkdir(parents=True)
        (root / 'bin').mkdir()
        (root / 'bin/steam_appid.txt').write_text('234140\n')
        tab = struct.pack('<II', 2048, 0) + struct.pack('<IIII', SKELETON_HASH, 0, len(skeleton), len(skeleton))
        (archives / 'game0.tab').write_bytes(tab)
        arc = archives / 'game0.arc'
        arc.write_bytes(skeleton)
        db_path = Path(temp) / 'hashes.db'
        with sqlite3.connect(db_path) as db:
            db.executescript('''
                CREATE TABLE kv (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,
                    v TEXT NOT NULL, PRIMARY KEY(lookup3,murmur));
                CREATE TABLE files (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,
                    name TEXT, size INTEGER NOT NULL, parent INTEGER NOT NULL DEFAULT 0,
                    PRIMARY KEY(lookup3,murmur));
                PRAGMA user_version=1;
            ''')
            db.execute('INSERT INTO kv VALUES (?,0,?)', (SKELETON_HASH, SKELETON_NAME))
        output = Path(temp) / 'out'
        path = (output / SKELETON_NAME).with_suffix('.json')
        gltf_path = path.with_suffix('.gltf')

        def extract():
            return subprocess.run([str(binary), 'extract', str(root), f'0x{SKELETON_HASH:08X}',
                                   '--module', str(module), '-d', str(db_path), '-o', str(output)],
                                  cwd=temp, capture_output=True, text=True)

        result = extract()
        assert result.returncode == 0, (result.stdout, result.stderr)
        value = json.loads(path.read_text())
        assert value['name']['stringAndFlag'] == 'Reference'
        assert value['bones'] == [{'name': {'stringAndFlag': 'hip'}, 'lockTranslation': False}]
        assert value['parentIndices'] == [-1]
        assert value['referencePose'][0]['translation'] == [1.0, 2.0, 3.0, 0.0]
        assert value['referencePose'][0]['rotation']['vec'] == [0.0, 0.0, 0.0, 1.0]
        assert value['referencePose'][0]['scale'] == [1.0, 1.0, 1.0, 0.0]
        gltf = json.loads(gltf_path.read_text())
        assert len(gltf['skins']) == 1 and len(gltf['skins'][0]['joints']) == 1
        joint = gltf['skins'][0]['joints'][0]
        assert gltf['nodes'][joint]['name'] == 'hip'
        assert not gltf.get('meshes')

        # A corrupt local fixup must leave neither skeleton JSON nor glTF.
        path.unlink()
        gltf_path.unlink()
        damaged = bytearray(skeleton)
        data_start = struct.unpack_from('<I', damaged, 192 + 20)[0]
        local_offset = struct.unpack_from('<I', damaged, 192 + 24)[0]
        struct.pack_into('<I', damaged, data_start + local_offset + 4, 0xFFFFFFFF)
        arc.write_bytes(damaged)
        result = extract()
        assert result.returncode != 0, (result.stdout, result.stderr)
        assert not path.exists()
        assert not gltf_path.exists()

        # A linked skeleton whose virtual class is another registered type is invalid.
        damaged = bytearray(skeleton)
        class_start = struct.unpack_from('<I', damaged, 64 + 20)[0]
        animation_class = damaged.index(b'hkaAnimationContainer\0', class_start) - class_start
        virtual_offset = struct.unpack_from('<I', damaged, 192 + 32)[0]
        struct.pack_into('<I', damaged, data_start + virtual_offset + 2 * 12 + 8, animation_class)
        arc.write_bytes(damaged)
        result = extract()
        assert result.returncode != 0, (result.stdout, result.stderr)
        assert 'unexpected or missing virtual class' in result.stderr
        assert not path.exists()
        assert not gltf_path.exists()

        # An unlisted hash must keep JSON and glTF together at the numeric fallback path.
        arc.write_bytes(skeleton)
        with sqlite3.connect(db_path) as db:
            db.execute('DELETE FROM kv WHERE lookup3 = ?', (SKELETON_HASH,))
        result = extract()
        assert result.returncode == 0, (result.stdout, result.stderr)
        unknown_path = output / f'{SKELETON_HASH:08X}'
        assert json.loads(unknown_path.with_suffix('.json').read_text())['bones'][0]['name']['stringAndFlag'] == 'hip'
        assert len(json.loads(unknown_path.with_suffix('.gltf').read_text())['skins'][0]['joints']) == 1

    print('Mad Max Havok skeleton decode passed')


if __name__ == '__main__':
    main()
